#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <map>
#include <z3++.h>
#include "validity/validity.h" // Your custom checker header

using namespace std;
using namespace std::chrono;

// Remove spaces from the input formula
string clean_formula(const string &s)
{
    string res = "";
    for (char ch : s)
    {
        if (!isspace(ch))
            res += ch;
    }
    return res;
}

// --- Z3 Parsing Engine (Matches strict BNF & prevents segfaults on uppercase) ---
z3::expr parse_z3(const string &s, int &pos, z3::context &c, map<char, z3::expr> &vars)
{
    if (pos >= (int)s.length())
        throw runtime_error("Unexpected end of string");

    if (s[pos] == '~')
    {
        pos++;
        return !parse_z3(s, pos, c, vars);
    }
    else if (s[pos] == '(')
    {
        pos++;
        z3::expr left = parse_z3(s, pos, c, vars);
        char op = s[pos++];
        z3::expr right = parse_z3(s, pos, c, vars);
        pos++; // skip ')'

        if (op == '|')
            return left || right;
        if (op == '&')
            return left && right;
        if (op == '>')
            return z3::implies(left, right);
        if (op == '^')
            return left != right;
        throw runtime_error(string("Unknown operator: ") + op);
    }
    else
    {
        // Safe map lookup for ANY character ('a'-'z', 'A'-'Z', etc.)
        char var_name = s[pos++];
        if (vars.find(var_name) == vars.end())
        {
            vars.insert({var_name, c.bool_const(string(1, var_name).c_str())});
        }
        return vars.at(var_name);
    }
}

int main()
{
    ifstream infile("testcases.txt");
    ofstream outfile("results.csv");

    if (!infile)
    {
        cerr << "Failed to open testcases.txt\n";
        return 1;
    }

    outfile << "ID,Length,Custom_Result,Z3_Result,Custom_Time_us,Z3_Time_us,Match\n";

    cout << left << setw(5) << "ID" << setw(10) << "Length"
         << setw(16) << "Custom(CNF)" << setw(14) << "Z3 SMT"
         << setw(15) << "Custom (us)" << setw(15) << "Z3 (us)"
         << setw(8) << "Match" << "\n";
    cout << string(83, '-') << "\n";

    z3::context c;
    string line;
    int id = 1;

    while (getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        string formula = clean_formula(line);
        if (formula.empty())
            continue;

        // 1. Time Your Custom CNF Validator
        auto start_custom = high_resolution_clock::now();
        bool custom_valid = false;
        try
        {
            custom_valid = isFormulaValid(formula);
        }
        catch (const exception &e)
        {
            cerr << "Custom checker error on ID " << id << ": " << e.what() << "\n";
            continue;
        }
        auto stop_custom = high_resolution_clock::now();
        auto dur_custom = duration_cast<microseconds>(stop_custom - start_custom).count();

        // 2. Time Z3 Solver
        auto start_z3 = high_resolution_clock::now();
        int pos = 0;
        bool z3_valid = false;
        try
        {
            // Reset map for each formula to keep memory clean
            map<char, z3::expr> z3_vars;
            z3::expr z3_form = parse_z3(formula, pos, c, z3_vars);

            z3::solver s(c);
            s.add(!z3_form); // Check unsat of negation
            z3_valid = (s.check() == z3::unsat);
        }
        catch (const exception &e)
        {
            cerr << "Z3 error on ID " << id << ": " << e.what() << "\n";
            continue;
        }
        auto stop_z3 = high_resolution_clock::now();
        auto dur_z3 = duration_cast<microseconds>(stop_z3 - start_z3).count();

        bool match = (custom_valid == z3_valid);
        string c_res = custom_valid ? "VALID" : "INVALID";
        string z_res = z3_valid ? "VALID" : "INVALID";

        cout << left << setw(5) << id << setw(10) << formula.length()
             << setw(16) << c_res << setw(14) << z_res
             << setw(15) << dur_custom << setw(15) << dur_z3
             << setw(8) << (match ? "YES" : "NO") << "\n";

        outfile << id << "," << formula.length() << "," << c_res << "," << z_res << ","
                << dur_custom << "," << dur_z3 << "," << (match ? "1" : "0") << "\n";
        id++;
    }

    infile.close();
    outfile.close();
    cout << string(83, '-') << "\n";
    cout << "Execution completed. Results saved to results.csv\n";
    return 0;
}