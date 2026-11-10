#!/usr/bin/env python3
"""Keep only YOUR functions in a folded-stack file (stdin -> stdout).

Every frame that is not one of your functions (std::, malloc, libc, loader, ...) is removed.
The time spent inside those frames is charged to the nearest of your functions above them,
so e.g. CNF::merge shows up wide because of the std::set work it triggers, instead of
being hidden under a pile of _Rb_tree / allocator boxes.

"Your functions" = text symbols in the binary whose source file is not under /usr, /lib or /opt.
They are read from `nm -l` output (needs the -g build that profile.sh makes).
If no such symbols are found, a deny-list of library name patterns is used instead.

Usage:
  nm -l --defined-only ./program | c++filt -p > symbols.txt
  python3 filter_user_frames.py --symbols symbols.txt [--summary report.txt] [--extra REGEX] \
      < out.raw.folded > out.user.folded
"""

import argparse
import re
import sys
from collections import defaultdict

# Compiler-generated clones: foo.cold, foo.part.0, foo.constprop.0, ...
CLONE_SUFFIX = re.compile(r"(\.(cold|part|constprop|isra|lto_priv)(\.\d+)*)+$")
CLONE_TAG = re.compile(r" \[clone [^\]]*\]")

# Fallback when the symbol table has no usable source paths.
DENY = re.compile(
    r"^(std::|__gnu_cxx::|__cxx|_IO_|_IO|_dl_|__libc_|__GI_|_int_|_start$|_init$|_fini$|call_init|"
    r"cfree|malloc|free$|calloc|realloc|operator (new|delete)|mem(cpy|move|set|chr)|__mem|__str|"
    r"__cxa|_Unwind|__x86|__pthread|\[|\(anonymous namespace\)::__)"
)


def load_user_symbols(path):
    names = set()
    with open(path, errors="replace") as fh:
        for line in fh:
            line = line.rstrip("\n")
            if "\t" not in line:
                continue
            left, right = line.split("\t", 1)
            parts = left.split(" ", 2)
            if len(parts) < 3 or parts[1] not in ("T", "t", "W", "w"):
                continue
            src = right.rsplit(":", 1)[0]
            if src.startswith(("/usr/", "/lib", "/opt/")):
                continue
            names.add(CLONE_TAG.sub("", parts[2].strip()))
    return names


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--symbols",
        required=True,
        help="output of: nm -l --defined-only BIN | c++filt -p",
    )
    ap.add_argument(
        "--summary", help="write a per-function ranking (self / inclusive) to this file"
    )
    ap.add_argument(
        "--extra", default="", help="regex of additional frame names to keep"
    )
    args = ap.parse_args()

    user = load_user_symbols(args.symbols)
    heuristic = not user
    extra = re.compile(args.extra) if args.extra else None
    if heuristic:
        sys.stderr.write(
            "note: no user symbols with source paths found in nm output; "
            "falling back to a library-name deny-list\n"
        )
    else:
        sys.stderr.write(f"note: {len(user)} user functions found in the binary\n")

    def is_user(frame):
        if extra and extra.search(frame):
            return True
        if heuristic:
            return not DENY.match(frame)
        return CLONE_SUFFIX.sub("", frame) in user

    stacks = defaultdict(int)
    kept = dropped = 0
    for line in sys.stdin:
        line = line.rstrip("\n")
        if not line:
            continue
        stack, _, cnt = line.rpartition(" ")
        try:
            cnt = int(cnt)
        except ValueError:
            continue
        frames = [CLONE_SUFFIX.sub("", f) for f in stack.split(";") if is_user(f)]
        if not frames:
            dropped += cnt
            continue
        stacks[";".join(frames)] += cnt
        kept += cnt

    for stack, cnt in stacks.items():
        sys.stdout.write(f"{stack} {cnt}\n")

    total = kept + dropped
    pct = (100.0 * kept / total) if total else 0.0
    sys.stderr.write(
        f"samples: {kept} in your functions ({pct:.1f}%), "
        f"{dropped} outside them (startup, loader, exit)\n"
    )

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
            out.write(
                "'inclusive' = function plus everything it called (std:: included).\n"
            )
            out.write(
                "'self' = time in the function itself plus the std::/libc code it called directly.\n\n"
            )
            out.write(f"{'inclusive%':>11} {'self%':>7}  function\n")
            for fn, cnt in sorted(incl_t.items(), key=lambda kv: -kv[1])[:40]:
                out.write(
                    f"{100.0 * cnt / max(kept, 1):>10.1f}% {100.0 * self_t.get(fn, 0) / max(kept, 1):>6.1f}%  {fn}\n"
                )


if __name__ == "__main__":
    main()
