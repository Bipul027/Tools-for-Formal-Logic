#include "cnf/cnf.h"
#include "cdcl/cdcl.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#ifdef PROFILE_MODE
static constexpr bool kQuiet = true;
#else
static constexpr bool kQuiet = false;
#endif

using Clock = std::chrono::steady_clock;

struct StageTime
{
    const char *name;
    double sec;
};
static std::vector<StageTime> g_stages;
static Clock::time_point g_last = Clock::now();

static void lap(const char *name)
{
    auto now = Clock::now();
    g_stages.push_back({name, std::chrono::duration<double>(now - g_last).count()});
    g_last = Clock::now();
}

static bool skipped(const char *name)
{
    const char *e = std::getenv("SKIP");
    if (!e)
        return false;
    return (std::string(",") + e + ",").find(std::string(",") + name + ",") != std::string::npos;
}

static void printStages()
{
    double total = 0;
    for (const auto &s : g_stages)
        total += s.sec;
    std::fprintf(stderr, "\n------------- stage timings -------------\n");
    for (const auto &s : g_stages)
        std::fprintf(stderr, "%-26s %11.3f ms  %5.1f%%\n", s.name, s.sec * 1000.0,
                     total > 0 ? 100.0 * s.sec / total : 0.0);
    std::fprintf(stderr, "%-26s %11.3f ms\n", "TOTAL", total * 1000.0);
    std::fprintf(stderr, "-----------------------------------------\n");
}

static CNF parseDimacsCNF(const std::string &s)
{
    CNF cnf;
    std::set<int> clause;
    size_t i = 0;
    const size_t n = s.size();

    while (i < n)
    {
        while (i < n && (s[i] == ' ' || s[i] == ',' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
            i++;

        if (i >= n)
            break;

        if (s[i] == ';')
        {
            if (!clause.empty())
            {
                cnf.addClause(std::move(clause));
                clause.clear();
            }
            i++;
            continue;
        }

        bool neg = false;
        if (s[i] == '-')
        {
            neg = true;
            i++;
        }
        else if (s[i] == '+')
        {
            i++;
        }

        int var = 0;
        bool hasDigits = false;
        while (i < n && s[i] >= '0' && s[i] <= '9')
        {
            var = var * 10 + (s[i] - '0');
            hasDigits = true;
            i++;
        }

        if (!hasDigits)
        {
            i++;
            continue;
        }

        if (var > 0)
        {
            int lit = neg ? (2 * var + 1) : (2 * var);
            clause.insert(lit);
        }
    }

    if (!clause.empty())
        cnf.addClause(std::move(clause));

    return cnf;
}

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string s;
    std::getline(std::cin, s);
    lap("read input");

    CNF cnf = parseDimacsCNF(s);
    lap("parseDimacsCNF");

    Result SAT;
    double elapsed = 0;
    if (!skipped("cdcl"))
    {
        auto start = Clock::now();
        SAT = CDCLCNF(cnf);
        auto end = Clock::now();
        elapsed = std::chrono::duration<double>(end - start).count();
    }
    lap("CDCLCNF");
    auto start = Clock::now();
    Result NumSat = CountCNF(cnf);
    auto end = Clock::now();
    elapsed = std::chrono::duration<double>(end - start).count();
    lap("MODELCOUNTING");

    std::cout << "===============CDCL============" << "\n";
    std::cout << ((SAT.sat) ? "SATISFIABLE" : "UNSAT") << "\n";
    std::cout << "Conflicts: " << SAT.conflicts << "\n";
    std::cout << "Decisions: " << SAT.decisions << "\n";
    std::cout << "Num Variables: " << (SAT.model.empty() ? 0 : SAT.model.size() - 1) << "\n";

    std::cout << "===============MODEL COUNTING============" << "\n";
    std::cout << ((SAT.sat) ? "SATISFIABLE" : "UNSAT") << "\n";
    std::cout << "SAT assignments: " << NumSat.satAssignments << "\n";
    if (!kQuiet)
        std::cout << "CDCL Execution Time: " << elapsed << " seconds\n";
    else
        printStages();
    return 0;
}