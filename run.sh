# python3 gen_formula.py random --n 5000000 --vars 50000 > input.txt
python3 gen_formula.py sat3 --vars 200 --ratio 4.26 > sat.txt
./profile.sh sat.txt