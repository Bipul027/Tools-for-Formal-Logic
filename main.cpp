#include "evaluator/evaluator.h"
#include "cnf/converting_functions.h"
#include "validity/validity.h"
#include "tseiten/tseitin.h"
#include "dpll/dpll.h"
#include "cdcl/cdcl.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// Normal build: behaves like your original main.
// Profiling build (-DPROFILE_MODE): no printing and no print-only work (treeToStr, printTree, ...),
// and a per-stage timing table is written to stderr.
// The UNI_ONLY / NNF / convertToCNF / isCNFValid block is compiled out of the profiling build.
//   SKIP=cdcl        skip the CDCL solver at runtime
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

// Records the time since the previous lap() under `name`.
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

int main()
{
    // Unsynchronised iostreams: getline on a huge line no longer goes through stdio getc() per character.
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string s;
    std::getline(std::cin, s);
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    lap("read input");

    Formula f = Formula(s);
    lap("parse (Formula ctor)");

    Formula copy = f;
    (void)copy;
    lap("copy of Formula");

    Node *treeNode = f.root;
    if (!kQuiet)
        f.printTree();

    assignment a;
    a.buildModel(treeNode);
    lap("assignment::buildModel");
    if (!kQuiet)
    {
        std::cout << "===========Current Assignment=============" << "\n";
        a.printModel();
        std::cout << "=========Formula evaluation===========" << "\n";
    }

    auto evalResult = f.evaluateFormula(a, treeNode);
    lap("evaluateFormula");
    if (!kQuiet)
    {
        std::cout << "F: " << evalResult << std::endl;
        std::cout << "String: " << treeToStr(treeNode) << std::endl;
        std::cout << "===========Tseitin Encoding==========" << "\n";
    }
    (void)evalResult;

    std::pair<CNF, std::vector<std::string>> Tseitin_encoding = TseitinEncodedCNF(f);
    lap("TseitinEncodedCNF");
    if (!kQuiet)
        Tseitin_encoding.first.print(Tseitin_encoding.second);

    bool SAT = false;
    double elapsed = 0;
    if (!skipped("cdcl"))
    {
        auto start = Clock::now();
        SAT = CDCLCNF(Tseitin_encoding.first);
        auto end = Clock::now();
        elapsed = std::chrono::duration<double>(end - start).count();
    }
    lap("CDCLCNF");
    if (!kQuiet)
    {
        std::cout << "===============CDCL============" << "\n";
        std::cout << ((SAT) ? "SATISFIABLE" : "UNSAT") << "\n";
    }
    else
    {
        std::fprintf(stderr, "CDCL result: %s\n", skipped("cdcl") ? "skipped" : (SAT ? "SATISFIABLE" : "UNSAT"));
    }

#ifndef PROFILE_MODE
    // Not part of the profiling build: UNI_ONLY / NNF / convertToCNF / isCNFValid
    {
        Node *UNI_NODE = UNI_ONLY(treeNode);
        lap("UNI_ONLY");
        if (!kQuiet)
            std::cout << "Universal String: " << treeToStr(UNI_NODE) << std::endl;

        Node *NNF_NODE = NNF(UNI_NODE);
        lap("NNF");
        if (!kQuiet)
        {
            std::cout << "NNF String: " << treeToStr(NNF_NODE) << std::endl;
            std::cout << "CNF String: ";
        }

        CNF Formula_in_CNF = convertToCNF(NNF_NODE);
        lap("convertToCNF");
        if (!kQuiet)
            Formula_in_CNF.print(f.propIdMap);

        auto validity = isCNFValid(Formula_in_CNF);
        lap("isCNFValid");
        if (!kQuiet)
            std::cout << "Validity: " << validity << "\n";
        else
            (void)validity;
    }
#endif

    if (!kQuiet)
        std::cout << "CDCL Execution Time: " << elapsed << " seconds\n";
    else
        printStages();
    return 0;
}