#include "count.h"

static int conflicts = 0;
static int decisions = 0;
static cpp_int satAssignments = 0;
static int freeVariables = 0;
static std::vector<int8_t> phase;

using namespace CDCLnamespace;
// The list of clauses each literal is present in.
// using Occurrences = std::vector<std::vector<int>>;

enum Assignment
{
    UNASSIGNED = -1,
    FALSE = 0,
    TRUE = 1
};

static inline int litValue(const Model &model, int lit)
{
    const int v = model[lit >> 1];
    return v == UNASSIGNED ? UNASSIGNED : (v ^ (lit & 1));
}

static inline void Assign(int lit, Dstack &dstack, int8_t *val)
{
    val[lit] = TRUE;
    val[lit ^ 1] = FALSE;
    dstack.push_back(lit);
}

// static inline void AssignNew(int lit, Model &model, Dstack &dstack, int &satisfiedClauses)
// {
//     Assign(lit, model, dstack);
// }

bool InitializeWatches(Clauses &clauses, Dstack &dstack, Watches &watches, int8_t *val)
{
    size_t numClauses = 0;
    for (size_t i = 0; i < clauses.size(); i++)
    {
        // Assumes the literals in c are sorted
        Clause c = std::move(clauses[i]);
        c.erase(std::unique(c.begin(), c.end()), c.end());
        bool tautology = false;
        for (size_t k = 0; k + 1 < c.size(); k++)
            if ((c[k] ^ 1) == c[k + 1])
            {
                tautology = true;
                break;
            }

        if (tautology)
            continue;
        if (c.empty())
            return false;
        if (c.size() == 1)
        {
            const int8_t value = val[c[0]];
            if (value == FALSE)
                return false;
            if (value == UNASSIGNED)
                Assign(c[0], dstack, val);
            continue;
        }
        clauses[numClauses] = std::move(c);
        const Clause &cur = clauses[numClauses];
        watches[cur[0]].push_back({static_cast<int>(numClauses), cur[1]});
        watches[cur[1]].push_back({static_cast<int>(numClauses), cur[0]});
        numClauses++;
    }
    clauses.resize(numClauses);
    return true;
}

static int UnitPropagate(Clauses &clauses, Dstack &dstack, Watches &watches, size_t &head, int8_t *val)
{
    while (head < dstack.size())
    {
        const int falseLit = dstack[head++] ^ 1;
        std::vector<Watcher> &watch = watches[falseLit];
        size_t n = watch.size();
        size_t i = 0, j = 0;
        while (i < n)
        {
            const Watcher w = watch[i++];
            // The clause is sat hence do not check
            if (val[w.blocker] == TRUE)
            {
                watch[j] = w;
                j++;
                continue;
            }

            Clause &clause = clauses[w.clause_index];

            // falseLit will be clause[1] if it were being watched already
            if (clause[0] == falseLit)
                std::swap(clause[0], clause[1]);

            const int first = clause[0];

            // If clause[0] isn't the watcher's blocker and also it evaluated to true then clause is sat and hence skip scanning
            if (first != w.blocker && val[first] == TRUE)
            {
                watch[j++] = {w.clause_index, first};
                continue;
            }

            // Search for a non-false literal in the clause to watch and swap with clause[1]
            bool found = false;
            for (size_t k = 2; k < clause.size(); k++)
            {
                if (val[clause[k]] != FALSE)
                {
                    std::swap(clause[1], clause[k]);
                    watches[clause[1]].push_back({w.clause_index, first});
                    found = true;
                    break;
                }
            }

            if (found)
                continue;

            watch[j++] = {w.clause_index, first};

            // Conflict found
            if (val[first] == FALSE)
            {
                while (i < n)
                    watch[j++] = watch[i++];
                watch.resize(j);
                head = dstack.size();
                return w.clause_index;
            }
            // If no conflict assign first to true
            Assign(first, dstack, val);
        }
        watch.resize(j);
    }
    return -1;
}

static int Decide(const int8_t *val, const Clause &clause)
{
    for (const int &lit : clause)
        if (val[lit] == UNASSIGNED)
            return lit >> 1;
    return 0;
}

Result CountClauses(Clauses &clauses, int propNums)
{
    conflicts = 0;
    decisions = 0;
    satAssignments = 0;
    freeVariables = propNums;
    Model model(propNums + 1, UNASSIGNED);
    Dstack dstack;
    Dstacklevel dstacklevel;
    size_t unsat_head = 0;
    std::vector<size_t> head_stack;
    head_stack.reserve(propNums + 1);
    Watches watches(2 * (propNums + 1));
    size_t head = 0;
    int dlevel = 0;
    phase.assign(propNums + 1, TRUE);
    std::vector<int8_t> values(2 * (propNums + 1), UNASSIGNED);
    int8_t *const val = values.data();

    if (!InitializeWatches(clauses, dstack, watches, val))
        return {false, model, conflicts, decisions, 0};
    // go back to the last unflipped decision level, flip it update dstacklevel, dstack, dlevel
    int numClauses = clauses.size();
    auto backtrack = [&]() -> bool
    {
        while (dlevel > 0)
        {
            size_t start = dstacklevel[dlevel - 1];
            int lit = dstack[start];
            if (lit & 1)
            {
                while (dstack.size() > start)
                {
                    int l = dstack.back();
                    val[l] = UNASSIGNED;
                    val[l ^ 1] = UNASSIGNED;
                    dstack.pop_back();
                }
                dstacklevel.pop_back();
                head_stack.pop_back();
                dlevel--;
                continue;
            }
            while (dstack.size() > start)
            {
                int l = dstack.back();
                val[l] = UNASSIGNED;
                val[l ^ 1] = UNASSIGNED;
                dstack.pop_back();
            }
            unsat_head = head_stack.back();
            head = start;
            Assign(lit | 1, dstack, val);
            return true;
        }
        return false;
    };

    while (true)
    {
        int conflict = UnitPropagate(clauses, dstack, watches, head, val);
        if (conflict != -1)
        {
            if (!backtrack())
                break;
            continue;
        }
        while (unsat_head < numClauses)
        {
            uint8_t clause_sat = false;
            for (const int &lit : clauses[unsat_head])
            {
                if (val[lit] == TRUE)
                {
                    clause_sat = true;
                    unsat_head++;
                    break;
                }
            }
            if (!clause_sat)
                break;
        }

        int decisionId = 0;
        if (unsat_head != clauses.size())
            decisionId = Decide(val, clauses[unsat_head]);

        if (decisionId == 0)
        {
            freeVariables = propNums - static_cast<int>(dstack.size());
            satAssignments += cpp_int(1) << freeVariables;
            if (!backtrack())
                break;
            continue;
        }
        // Assign true to the variable
        dlevel++;
        dstacklevel.push_back(dstack.size());
        head_stack.push_back(unsat_head);
        Assign(2 * decisionId, dstack, val);
    }
    return {satAssignments > 0, model, conflicts, decisions, satAssignments};
}

Result Count(const Formula &formula)
{
    std::pair<CNF, std::vector<std::string>> TseitinEncoded = TseitinEncodedCNF(formula);
    return CountCNF(TseitinEncoded.first);
}

Result CountCNF(const CNF &F)
{
    const auto &set_clauses = F.getClauses();
    Clauses clauses;
    clauses.reserve(set_clauses.size());
    for (const auto &s : set_clauses)
        clauses.emplace_back(s.begin(), s.end());
    return CountClauses(clauses, F.propNums());
}