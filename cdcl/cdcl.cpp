#include "vsids.h"
#include <queue>

static int conflicts = 0;
static int decisions = 0;
static cpp_int satAssignments = 0;
static int freeVariables = 0;
static std::vector<int8_t> phase;

using namespace CDCLnamespace;

enum Assignment
{
    UNASSIGNED = -1,
    FALSE = 0,
    TRUE = 1
};

struct Conflict
{
    Clause learntClause;
    int Id;
    int backtrackLevel;
};

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

int cdclevaluateClause(const Clause &clause, const Model &model)
{
    // return -1 if the clause is unassigned, 0 if false, 1 if true
    int res = 0;
    bool atleast_one_true = false;
    for (const int &varId : clause)
    {
        if (model.at(varId / 2) == UNASSIGNED)
            res = -1;
        else if ((model.at(varId / 2) ^ varId) & 1)
        {
            atleast_one_true = true;
            break;
        }
    }
    return (atleast_one_true ? 1 : res);
}

// evaluates a CNF under a partial model, returns 1 if true, 0 if false, -1 if unassigned
int evaluateCNF(const Clauses &clauses, const Model &model)
{

    bool unassigned = false;
    for (const auto &clause : clauses)
    {
        int res = cdclevaluateClause(clause, model);
        if (res == FALSE)
            return FALSE;
        if (res == UNASSIGNED)
            unassigned = true;
    }
    return unassigned ? UNASSIGNED : TRUE;
}

bool isModelPartial(const std::unordered_map<int, int> &model)
{
    for (const auto &[id, assignment] : model)
        if (assignment == UNASSIGNED)
            return true;
    return false;
}

/** Does the Unit Propagations at decision level 0 and assigns them respectively and removes the unit clauses
 * Sets up two watched literals as clause[0], clause[1] for every non-unit clause and tautologies
 * Returns false if there is a conflict at level 0.
 */
static bool InitializeWatches(Clauses &clauses, Model &model, Level &level, Reason &reason, Dstack &dstack, Watches &watches)
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

/* UnitPropagates a CNF and modifies the respective details and returns the index of the clause which is causing a conflict
 * Returns -1 if no conflict exists.
 * dstack[head] are the assignments whose consequences have not been propagated yet.
 * Every clause is watched by clause[0] and clause[1], If one watch is false, the other watch is true, or no unassigned
 * replacement exists.
 * Whenever a clause becomes the reason of a literal, then that literal sits at clause[0].
 */
static int UnitPropagate(Clauses &clauses, Model &model, Level &level, Reason &reason, Dstack &dstack, Watches &watches, size_t &head, int dlevel)
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
            Assign(first, dlevel, w.clause_index, model, level, reason, dstack);
        }
        watch.resize(j);
    }
    return -1;
}

/**
 * First UIP conflict analysis
 *
 */
static Conflict AnalyzeConflict(Clauses &clauses, Clause &clause, Model &model, Level &level, Reason &reason, Dstack &dstack, VSIDSHeap &vsids, int dlevel)
{
    Conflict conflict_analysis;
    conflict_analysis.learntClause = std::vector<int>();
    conflict_analysis.backtrackLevel = dlevel;
    int btLevel = 0;
    static thread_local std::vector<char> seen;

    if (seen.size() < model.size())
        seen.resize(model.size(), 0);

    int count = 0;
    int p = -1;

    const Clause *cur = &clause;
    int dstackIdx = static_cast<int>(dstack.size()) - 1;

    while (true)
    {
        for (const auto &varId : *cur)
        {
            int v = varId >> 1;
            if (p != -1 && varId / 2 == p / 2)
                continue;
            if (seen[v] == false && level[v] > 0)
            {
                seen[v] = true;
                if (level[v] == dlevel)
                    count++;
                else
                {
                    conflict_analysis.learntClause.push_back(varId);
                    btLevel = std::max(btLevel, level[v]);
                }
            }
        }
        while (dstackIdx >= 0 && seen[dstack[dstackIdx] / 2] == false)
        {
            dstackIdx--;
        }
        p = dstack[dstackIdx--];
        seen[p >> 1] = false;
        if (--count == 0)
            break;
        cur = &clauses[reason[p >> 1]];
    }

    for (int Id : conflict_analysis.learntClause)
        seen[Id >> 1] = 0;
    conflict_analysis.Id = p ^ 1;
    conflict_analysis.learntClause.push_back(p ^ 1);
    conflict_analysis.backtrackLevel = btLevel;
    conflicts++;
    for (int &lit : conflict_analysis.learntClause)
        vsids.increaseScore(lit >> 1);
    vsids.decayInc();
    return conflict_analysis;
}

static void Backtrack(int target_level, int &dlevel, Model &model, Level &level, Reason &reason, Dstack &dstack, Dstacklevel &dstacklevel, VSIDSHeap &vsids, size_t &head)
{
    if (target_level >= dlevel)
        return;
    int cut = dstacklevel[target_level];
    for (int i = static_cast<int>(dstack.size()) - 1; i >= cut; i--)
    {
        int v = dstack[i] / 2;

        phase[v] = model[v];
        model[v] = UNASSIGNED;
        level[v] = 0;
        reason[v] = -1;
        vsids.insert(v);
    }

    dstack.resize(cut);
    dstacklevel.resize(target_level);
    dlevel = target_level;
    head = dstack.size();
}

// Decides which proposition to make a decision and returns it based off the maximum score
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

Result CDCLClauses(Clauses &clauses, int propNums)
{
    conflicts = 0;
    decisions = 0;
    // satAssignments = 0;
    // freeVariables = propNums;

    Model model(propNums + 1, UNASSIGNED);
    Level level(propNums + 1, 0);
    // index of the clause in clauses vector which is responsible for assigning a variable of id i;
    Reason reason(propNums + 1, -1);
    // stores every assignment in order from the start
    Dstack dstack;
    // dstack_level[i] returns an index in dstack from which the decision level i started
    Dstacklevel dstacklevel;
    Watches watches(2 * (propNums + 1));
    size_t head = 0;
    int dlevel = 0;

    VSIDSHeap vsids(propNums);
    phase.assign(propNums + 1, TRUE);

    if (!InitializeWatches(clauses, model, level, reason, dstack, watches))
        return {false, model, conflicts, decisions, 0};

    while (true)
    {
        int conflict = UnitPropagate(clauses, model, level, reason, dstack, watches, head, dlevel);
        if (conflict != -1)
        {
            if (dlevel == 0)
                return {false, model, conflicts, decisions, satAssignments};
            Conflict conflictAnalysis = AnalyzeConflict(clauses, clauses[conflict], model, level, reason, dstack, vsids, dlevel);
            Clause &learntClause = conflictAnalysis.learntClause;
            if (learntClause.size() > 1)
            {
                std::swap(learntClause[0], learntClause.back());
                size_t best = 1;
                for (size_t k = 2; k < learntClause.size(); k++)
                    if (level[learntClause[k] >> 1] > level[learntClause[best] >> 1])
                        best = k;
                std::swap(learntClause[1], learntClause[best]);
            }
            Backtrack(conflictAnalysis.backtrackLevel, dlevel, model, level, reason, dstack, dstacklevel, vsids, head);
            int Id = conflictAnalysis.Id;
            if (conflictAnalysis.learntClause.size() == 1)
                Assign(Id, dlevel, -1, model, level, reason, dstack);
            else
            {
                clauses.push_back(std::move(conflictAnalysis.learntClause));
                const Clause &learnt = clauses[static_cast<int>(clauses.size()) - 1];
                watches[learnt[0]].push_back({static_cast<int>(clauses.size()) - 1, learnt[1]});
                watches[learnt[1]].push_back({static_cast<int>(clauses.size()) - 1, learnt[0]});
                Assign(Id, dlevel, static_cast<int>(clauses.size()) - 1, model, level, reason, dstack);
            }
            continue;
        }

        int decisionId = Decide(model, vsids);
        if (decisionId == 0)
            return {true, model, conflicts, decisions, 0};

        dlevel++;
        dstacklevel.push_back(dstack.size());
        Assign((2 * decisionId) ^ (phase[decisionId] ^ 1), dlevel, -1, model, level, reason, dstack);
    }
}

Result CDCL(const Formula &formula)
{
    std::pair<CNF, std::vector<std::string>> TseitinEncoded = TseitinEncodedCNF(formula);
    return CDCLCNF(TseitinEncoded.first);
}

Result CDCLCNF(const CNF &F)
{
    const auto &set_clauses = F.getClauses();
    Clauses clauses;
    clauses.reserve(set_clauses.size());
    for (const auto &s : set_clauses)
        clauses.emplace_back(s.begin(), s.end());
    return CDCLClauses(clauses, F.propNums());
}
