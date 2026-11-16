#!/usr/bin/env python3
import argparse
import re
import sys
from collections import defaultdict

CLONE_SUFFIX = re.compile(r"(\.(cold|part|constprop|isra|lto_priv)(\.\d+)*)+$")
CLONE_TAG = re.compile(r" \[clone [^\]]*\]")


# Strip template parameters <...> so std::vector<int, std::allocator<int>>::push_back becomes std::vector::push_back
def strip_templates(s):
    out = []
    depth = 0
    for ch in s:
        if ch == "<":
            depth += 1
        elif ch == ">":
            if depth > 0:
                depth -= 1
        elif depth == 0:
            out.append(ch)
    return "".join(out)


def normalize(name):
    name = CLONE_TAG.sub("", name).strip()
    name = CLONE_SUFFIX.sub("", name)
    # Strip function arguments (...)
    depth = 0
    for i in range(len(name) - 1, -1, -1):
        if name[i] == ")":
            depth += 1
        elif name[i] == "(":
            depth -= 1
            if depth == 0:
                if not name.startswith("(anonymous namespace)", i):
                    name = name[:i]
                break
    name = strip_templates(name).strip()
    # Strip return types from nm output (e.g. "bool CDCLCNF" -> "CDCLCNF")
    if " " in name and not name.startswith("operator"):
        parts = name.split()
        if not parts[-1].startswith("operator"):
            name = parts[-1]
    return name


def simplify_std(raw_name):
    """Turn ugly STL/allocation frames into a high-level label, or None if it's internal plumbing."""
    s = strip_templates(CLONE_TAG.sub("", raw_name)).strip()
    # Strip parameters
    if "(" in s:
        s = s[: s.find("(")].strip()

    # Memory allocation
    if s in (
        "operator new",
        "operator new[]",
        "malloc",
        "calloc",
        "realloc",
        "free",
        "operator delete",
    ):
        return "malloc/new (heap alloc)"

    if not s.startswith("std::"):
        return None

    # Map common ugly libstdc++ internals to clean container names
    s = s.replace("std::__cxx11::basic_string", "std::string")
    s = s.replace("std::basic_string", "std::string")
    s = s.replace("std::_Rb_tree", "std::set/map")
    s = s.replace("std::_Hashtable", "std::unordered_map/set")
    s = s.replace("std::__detail::_Map_base", "std::unordered_map")

    parts = s.split("::")
    if len(parts) < 2:
        return None

    # Drop low-level allocator/iterator/char_traits noise completely
    if any(
        k in parts[1]
        for k in ("allocator", "char_traits", "__", "_Iter", "_Vector_base", "_Bit_")
    ):
        return None

    # Keep container + high-level method (or summarize _M_ internals under the container name)
    container = f"std::{parts[1]}"
    if len(parts) >= 3:
        method = parts[2]
        if method.startswith("_"):
            return f"{container} (internal/resize/copy)"
        return f"{container}::{method}"
    return container


def load_user_symbols(path):
    names = set()
    with open(path, errors="replace") as fh:
        for line in fh:
            line = line.rstrip("\n")
            if "\t" not in line:
                continue
            left, right = line.split("\t", 1)
            parts = left.split(None, 2)
            if len(parts) < 3 or parts[1] not in ("T", "t", "W", "w"):
                continue
            src = right.rsplit(":", 1)[0]
            if src.startswith(("/usr/", "/lib", "/opt/")):
                continue
            names.add(normalize(parts[2]))
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--symbols", required=True)
    ap.add_argument("--summary")
    ap.add_argument("--dropped")
    ap.add_argument("--extra", default="")
    ap.add_argument("--top", type=int, default=150)
    args = ap.parse_args()

    user = load_user_symbols(args.symbols)
    extra = re.compile(args.extra) if args.extra else None
    sys.stderr.write(
        f"note: {len(user)} user functions listed in the binary's symbol table\n"
    )

    data = []
    for line in sys.stdin:
        line = line.rstrip("\n")
        if not line:
            continue
        stack, _, cnt = line.rpartition(" ")
        try:
            data.append((stack.split(";"), int(cnt)))
        except ValueError:
            continue

    stacks = defaultdict(int)
    gone = defaultdict(int)
    kept = dropped = 0

    for frames, cnt in data:
        out = []
        in_std_chain = False
        for f in frames:
            norm = normalize(f)
            if (extra and extra.search(f)) or (norm in user):
                out.append(norm)
                in_std_chain = False
            else:
                # Only keep the FIRST high-level std:: / alloc call after a user function
                # so std:: doesn't stack 10 levels deep!
                if not in_std_chain and out:
                    clean_std = simplify_std(f)
                    if clean_std:
                        out.append(clean_std)
                        in_std_chain = True
                        continue
                gone[f] += cnt

        if not out:
            dropped += cnt
            continue
        stacks[";".join(out)] += cnt
        kept += cnt

    distinct = {fr for s in stacks for fr in s.split(";")}
    for stack, cnt in stacks.items():
        sys.stdout.write(f"{stack} {cnt}\n")

    total = kept + dropped
    pct = (100.0 * kept / total) if total else 0.0
    sys.stderr.write(
        f"filter: user + 1-level std::; {len(distinct)} distinct functions in the graph\n"
    )
    sys.stderr.write(
        f"samples: {kept} in your functions ({pct:.1f}%), {dropped} outside them\n"
    )

    if args.dropped:
        with open(args.dropped, "w") as out:
            for fn, cnt in sorted(gone.items(), key=lambda kv: -kv[1])[:60]:
                out.write(f"{cnt:>9}  {fn}\n")

    if args.summary:
        self_t = defaultdict(int)
        incl_t = defaultdict(int)
        for stack, cnt in stacks.items():
            frames = stack.split(";")
            self_t[frames[-1]] += cnt
            for fn in set(frames):
                incl_t[fn] += cnt
        with open(args.summary, "w") as out:
            out.write(
                f"Percentages are of the {kept} samples that were inside your functions.\n"
            )
            out.write(f"{'inclusive%':>11} {'self%':>7}  function\n")
            for fn, cnt in sorted(incl_t.items(), key=lambda kv: -kv[1])[: args.top]:
                out.write(
                    f"{100.0 * cnt / max(kept, 1):>10.1f}% {100.0 * self_t.get(fn, 0) / max(kept, 1):>6.1f}%  {fn}\n"
                )


if __name__ == "__main__":
    main()
