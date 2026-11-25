#include "count.h"

static int conflicts = 0;
static int decisions = 0;
static cpp_int satAssignments = 0;
static int freeVariables = 0;
static std::vector<int8_t> phase;

using namespace CDCLnamespace;
using Occurrences = std::vector<std::vector<int>>;

enum Assignment
{
    UNASSIGNED = -1,
    FALSE = 0,
    TRUE = 1
};

static inline void UpdateSatisfaction(int lit, int change, const Occurrences &occurrences, std::vector<int> &trueCount, int &satisfiedClauses)
{
    for (const int &clause_index : occurrences[lit])
    {
        int before = trueCount[clause_index];
        trueCount[clause_index] += change;

        if (before == 0 && trueCount[clause_index] > 0)
            satisfiedClauses++;
        else if (before > 0 && trueCount[clause_index] == 0)
            satisfiedClauses--;
    }
}

static inline int litValue(const Model &model, int lit)
{
    const int v = model[lit >> 1];
    return v == UNASSIGNED ? UNASSIGNED : (v ^ (lit & 1));
}

static inline void Assign(int lit, int dlevel, int clause_index, Model &model, Level &level, Reason &reason, Dstack &dstack)
{
    const int v = lit >> 1;
    model[v] = (lit & 1) ^ 1;
    level[v] = dlevel;
    reason[v] = clause_index;
    dstack.push_back(lit);
}

static inline void AssignNew(int lit, int dlevel, int clause_index, Model &model, Level &level, Reason &reason, Dstack &dstack, const Occurrences &occurrences, std::vector<int> &propCount, int &satisfiedClauses)
{
    Assign(lit, dlevel, clause_index, model, level, reason, dstack);
    UpdateSatisfaction(lit, 1, occurrences, propCount, satisfiedClauses);
}

bool InitializeWatches(Clauses &clauses, Model &model, Level &level, Reason &reason, Dstack &dstack, Watches &watches)
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
            const int val = litValue(model, c[0]);
            if (val == FALSE)
                return false;
            if (val == UNASSIGNED)
                Assign(c[0], 0, -1, model, level, reason, dstack);
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

static int UnitPropagate(Clauses &clauses, Model &model, Level &level, Reason &reason, Dstack &dstack, Watches &watches, size_t &head, int dlevel, const Occurrences &occurrences, std::vector<int> &propCount, int &satisfiedClauses)
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
            if (litValue(model, w.blocker) == TRUE)
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
            if (first != w.blocker && litValue(model, first) == TRUE)
            {
                watch[j++] = {w.clause_index, first};
                continue;
            }

            // Search for a non-false literal in the clause to watch and swap with clause[1]
            bool found = false;
            for (size_t k = 2; k < clause.size(); k++)
            {
                if (litValue(model, clause[k]) != FALSE)
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
            if (litValue(model, first) == FALSE)
            {
                while (i < n)
                    watch[j++] = watch[i++];
                watch.resize(j);
                head = dstack.size();
                return w.clause_index;
            }
            // If no conflict assign first to true
            AssignNew(first, dlevel, w.clause_index, model, level, reason, dstack, occurrences, propCount, satisfiedClauses);
        }
        watch.resize(j);
    }
    return -1;
}

static int Decide(const Model &model, VSIDSHeap &vsids)
{
    while (!vsids.empty())
    {
        int lit = vsids.pop();
        if (model[lit] == UNASSIGNED)
        {
            decisions++;
            return lit;
        }
    }
    return 0;
}

Result CountClauses(Clauses &clauses, int propNums)
{
    conflicts = 0;
    decisions = 0;
    satAssignments = 0;
    freeVariables = propNums;

    Model model(propNums + 1, UNASSIGNED);
    Level level(propNums + 1, 0);
    Reason reason(propNums + 1, -1);
    Dstack dstack;
    Dstacklevel dstacklevel;
    Watches watches(2 * (propNums + 1));
    size_t head = 0;
    int dlevel = 0;
    VSIDSHeap vsids(propNums);
    phase.assign(propNums + 1, TRUE);

    if (!InitializeWatches(clauses, model, level, reason, dstack, watches))
        return {false, model, conflicts, decisions, 0};

    int numClauses = clauses.size();
    int satisfiedClauses = 0;
    Occurrences occurrences(2 * (propNums + 1));
    std::vector<int> propCount(numClauses, 0);

    for (int clause_index = 0; clause_index < (int)clauses.size(); clause_index++)
    {
        for (int lit : clauses[clause_index])
        {
            occurrences[lit].push_back(clause_index);

            if (litValue(model, lit) == TRUE)
                propCount[clause_index]++;
        }

        if (propCount[clause_index] > 0)
            satisfiedClauses++;
    }

    // go back to the last unflipped decision level, flip it update dstacklevel, dstack, dlevel
    auto backtrack = [&]() -> bool
    {
        while (dlevel > 0)
        {
            size_t start = dstacklevel[dlevel - 1];
            int lit = dstack[start];
            // If set to false then it is flipped
            if (lit & 1)
            {
                while (dstack.size() > start)
                {
                    const int assignedLit = dstack.back();
                    UpdateSatisfaction(assignedLit, -1, occurrences, propCount, satisfiedClauses);
                    model[dstack.back() / 2] = UNASSIGNED;
                    vsids.insert(dstack.back() / 2);
                    dstack.pop_back();
                }
                dstacklevel.pop_back();
                dlevel--;
                continue;
            }
            while (dstack.size() > start)
            {
                const int assignedLit = dstack.back();
                UpdateSatisfaction(assignedLit, -1, occurrences, propCount, satisfiedClauses);
                model[dstack.back() / 2] = UNASSIGNED;
                vsids.insert(dstack.back() / 2);
                dstack.pop_back();
            }
            head = start;
            AssignNew(lit | 1, dlevel, -1, model, level, reason, dstack, occurrences, propCount, satisfiedClauses);
            return true;
        }
        return false;
    };

    while (true)
    {
        int conflict = UnitPropagate(clauses, model, level, reason, dstack, watches, head, dlevel, occurrences, propCount, satisfiedClauses);
        if (conflict != -1)
        {
            if (!backtrack())
                break;
            continue;
        }

        int decisionId = 0;
        if (satisfiedClauses != (int)clauses.size())
            decisionId = Decide(model, vsids);

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
        AssignNew(2 * decisionId, dlevel, -1, model, level, reason, dstack, occurrences, propCount, satisfiedClauses);
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