# python3 gen_formula.py random --n 5000000 --vars 100000 > input.txt
# python3 gen_formula.py dimacs --vars 160 --ratio 4.26 > sat.txt
SKIP=cdcl ROOTS="CountCNF CountClauses UnitPropagate InitializeWatches Decide"  ./profile.sh sat.txt