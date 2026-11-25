# python3 gen_formula.py random --n 5000000 --vars 50000 > input.txt
python3 gen_formula.py dimacs --vars 10 --ratio 4.26 > sat.txt
./profile.sh sat.txt