#!/usr/bin/env python3
"""Generate large propositional formulas (one line, fully parenthesised) for profiling.

Operators emitted: & | > ^ ~   (the ones your Tseitin code handles)

Modes
  random  N binary operators over V atoms
  sat3    random 3-SAT: V variables, round(RATIO*V) clauses joined by &  (stresses CDCL)
  repeat  take an existing formula file and join N copies of it (syntax-agnostic scale-up)

Examples
  python3 gen_formula.py random --n 5000 --vars 40 > big.txt
  python3 gen_formula.py random --n 5000 --vars 40 --shape skewed > deep.txt
  python3 gen_formula.py sat3 --vars 80 > sat.txt
  python3 gen_formula.py repeat --n 2000 --formula-file small.txt > big.txt

If your parser rejects the atom names, change --atom-fmt (e.g. "p{}") or use
--atom-fmt letters (a..z, reused cyclically).
"""
import argparse
import random
import sys


def make_atom(fmt):
    if fmt == "letters":
        letters = "abcdefghijklmnopqrstuvwxyz"
        return lambda i: letters[i % 26]
    return lambda i: fmt.format(i + 1)


def combine(items, op_fn, shape, wrap):
    """Join items with binary operators iteratively (no recursion, any size)."""
    if shape == "skewed":
        acc = items[0]
        for x in items[1:]:
            acc = wrap("(" + acc + op_fn() + x + ")")
        return acc
    while len(items) > 1:
        nxt = []
        for i in range(0, len(items) - 1, 2):
            nxt.append(wrap("(" + items[i] + op_fn() + items[i + 1] + ")"))
        if len(items) % 2:
            nxt.append(items[-1])
        items = nxt
    return items[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=["random", "sat3", "repeat"])
    ap.add_argument("--n", type=int, default=1000, help="random: #binary operators; repeat: #copies")
    ap.add_argument("--vars", type=int, default=30, help="number of distinct atoms")
    ap.add_argument("--ratio", type=float, default=4.26, help="sat3: clauses per variable")
    ap.add_argument("--shape", choices=["balanced", "skewed"], default="balanced",
                    help="balanced = depth ~log n; skewed = depth ~n (deep recursion)")
    ap.add_argument("--ops", default="&|>^", help="random: binary operators to draw from")
    ap.add_argument("--neg", type=float, default=0.2, help="random: probability of negating a node")
    ap.add_argument("--atom-fmt", default="x{}", help='atom name format, e.g. "x{}" or "letters"')
    ap.add_argument("--rng-seed", type=int, default=1)
    ap.add_argument("--formula-file", help="repeat: file containing the formula to copy")
    ap.add_argument("--join", default="&", help="repeat: operator joining the copies")
    args = ap.parse_args()

    rng = random.Random(args.rng_seed)
    atom = make_atom(args.atom_fmt)
    if args.atom_fmt == "letters" and args.vars > 26:
        sys.stderr.write("note: 'letters' only has 26 distinct atoms; capping --vars at 26\n")
        args.vars = 26

    def maybe_neg(s):
        if rng.random() < args.neg and not s.startswith("~"):
            return "~" + s
        return s

    if args.mode == "random":
        leaves = [maybe_neg(atom(rng.randrange(args.vars))) for _ in range(args.n + 1)]
        ops = list(args.ops)
        formula = combine(leaves, lambda: rng.choice(ops), args.shape, maybe_neg)
    elif args.mode == "sat3":
        if args.vars < 3:
            sys.exit("sat3 needs --vars >= 3")
        clauses = []
        for _ in range(max(1, round(args.ratio * args.vars))):
            vs = rng.sample(range(args.vars), 3)
            lits = [("~" if rng.random() < 0.5 else "") + atom(v) for v in vs]
            clauses.append("(" + "|".join(lits) + ")")
        formula = combine(clauses, lambda: "&", args.shape, lambda s: s)
    else:
        if not args.formula_file:
            sys.exit("repeat needs --formula-file")
        with open(args.formula_file) as fh:
            seed = fh.read().strip().replace(" ", "")
        formula = combine(["(" + seed + ")"] * args.n, lambda: args.join, args.shape, lambda s: s)

    sys.stdout.write(formula + "\n")
    sys.stderr.write(f"generated {len(formula)} chars, mode={args.mode}, shape={args.shape}\n")


if __name__ == "__main__":
    main()