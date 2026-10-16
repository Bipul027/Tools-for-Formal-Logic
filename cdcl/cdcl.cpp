#include "cdcl.h"
namespace CDCLnamespace
{
    using Clause = std::set<int>;
    using Clauses = std::vector<Clause>;
    using Model = std::unordered_map<int, int>;
    using Level = std::unordered_map<int, int>;
    using Reason = std::unordered_map<int, int>;
    using Dstack = std::vector<int>;
    using Dstacklevel = std::vector<int>;
}

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

int cdclevaluateClause(const std::set<int> &clause, const std::unordered_map<int, int> &model)
{
    // return -1 if the clause is unassigned, 0 if false, 1 if true
    int res = 0;
    bool atleast_one_true = false;
    for (const int &varId : clause)
    {
        if (model.at(varId / 2) == UNASSIGNED)
            res = -1;
        else if ((model.at(varId / 2) ^ varId) % 2)
        {
            atleast_one_true = true;
            break;
        }
    }
    return (atleast_one_true ? 1 : res);
}

// evaluates a CNF under a partial model, returns 1 if true, 0 if false, -1 if unassigned
int evaluateCNF(const Clauses &clauses, const std::unordered_map<int, int> &model)
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

/* UnitPropagates a CNF and modifies the respective details and returns the index of the clause which is causing a conflict
 * Returns -1 if no conflict exists.
 */
int UnitPropagate(Clauses &clauses, Model &model, Level &level, Reason &reason, Dstack &dstack, int dlevel)
{
    bool propagate = true;
    while (propagate)
    {
        propagate = false;
        for (int clause_index = 0; clause_index < static_cast<int>(clauses.size()); clause_index++)
        {
            const Clause &clause = clauses[clause_index];
            int idTopropagate = 0;
            int num_unassigned = 0;
            bool sat = false;

            for (const auto &varId : clause)
            {
                if (model[varId / 2] == UNASSIGNED)
                {
                    num_unassigned++;
                    idTopropagate = varId;
                    continue;
                }
                if ((model[varId / 2] ^ varId) % 2)
                {
                    sat = true;
                    break;
                }
            }
            if (sat)
                continue;
            // if every literal is assigned and the clause is not sat then it is causing a conflict.
            if (num_unassigned == 0)
                return clause_index;
            if (num_unassigned == 1)
            {
                model[idTopropagate / 2] = (idTopropagate ^ 1) % 2;
                level[idTopropagate / 2] = dlevel;
                reason[idTopropagate / 2] = clause_index;
                dstack.push_back(idTopropagate);

                propagate = true;
            }
        }
    }
    return -1;
}

Conflict AnalyzeConflict(Clauses &clauses, Clause &clause, Model &model, Level &level, Reason &reason, Dstack &dstack, int dlevel)
{
    Conflict conflict_analysis;
    conflict_analysis.learntClause = std::set<int>();
    conflict_analysis.backtrackLevel = dlevel;
    int propNums = static_cast<int>(model.size());
    std::vector<bool> seen(propNums + 1, false);

    int count = 0;
    int p = -1;

    Clause cur = clause;
    int dstackIdx = static_cast<int>(dstack.size()) - 1;

    while (true)
    {
        for (const auto &varId : cur)
        {
            if (p != -1 && varId / 2 == p / 2)
                continue;
            if (seen[varId / 2] == false && level[varId / 2] > 0)
            {
                seen[varId / 2] = true;
                if (level[varId / 2] == dlevel)
                    count++;
                else
                    conflict_analysis.learntClause.insert(varId);
            }
        }
        while (dstackIdx >= 0 && seen[dstack[dstackIdx] / 2] == false)
        {
            dstackIdx--;
        }
        if (dstackIdx < 0)
        {
            throw std::logic_error(
                "AnalyzeConflict: could not find a relevant "
                "literal on the trail.");
        }
        p = dstack[dstackIdx];
        dstackIdx--;
        seen[p / 2] = false;
        count--;
        if (count > 0)
        {
            int reason_clause = reason.at(p / 2);

            if (reason_clause == -1)
            {
                throw std::logic_error(
                    "AnalyzeConflict: could not find a relevant "
                    "literal on the trail.");
            }
            cur = clauses[reason_clause];
        }
        if (count == 0)
            break;
    }
    conflict_analysis.Id = p ^ 1;
    conflict_analysis.learntClause.insert(p ^ 1);
    int btLevel = 0;
    for (auto &Id : (conflict_analysis.learntClause))
    {
        if (Id == (p ^ 1))
            continue;
        btLevel = std::max(btLevel, level[Id / 2]);
    }
    conflict_analysis.backtrackLevel = btLevel;
    return conflict_analysis;
}

void Backtrack(int target_level, int &dlevel, Model &model, Level &level, Reason &reason, Dstack &dstack, Dstacklevel &dstacklevel)
{
    if (target_level >= dlevel)
        return;
    int cut = dstacklevel[target_level];
    for (int i = static_cast<int>(dstack.size()) - 1; i >= cut; i--)
    {
        int v = dstack[i] / 2;

        model[v] = UNASSIGNED;
        level[v] = 0;
        reason[v] = -1;
    }

    dstack.resize(cut);
    dstacklevel.resize(target_level);
    dlevel = target_level;
}

int Decide(Model &model)
{
    for (int i = 1; i <= static_cast<int>(model.size()); i++)
    {
        if (model[i] == UNASSIGNED)
            return i;
    }
    return 0;
}

bool CDCLClauses(Clauses &clauses, int propNums)
{
    Model model;
    Level level;
    // index of the clause in clauses vector which is responsible for assigning a variable of id i;
    Reason reason;
    // stores every assignment in order from the start
    Dstack dstack;
    // dstack_level[i] returns an index in dstack from which the decision level i started
    Dstacklevel dstacklevel;

    int dlevel = 0;

    for (int i = 1; i <= propNums; i++)
    {
        model[i] = UNASSIGNED;
        level[i] = 0;
        reason[i] = -1;
    }

    while (true)
    {
        int conflict = UnitPropagate(clauses, model, level, reason, dstack, dlevel);
        if (conflict != -1)
        {
            if (dlevel == 0)
                return false;
            Conflict conflictAnalysis = AnalyzeConflict(clauses, clauses[conflict], model, level, reason, dstack, dlevel);
            Backtrack(conflictAnalysis.backtrackLevel, dlevel, model, level, reason, dstack, dstacklevel);
            clauses.push_back(conflictAnalysis.learntClause);
            int Id = conflictAnalysis.Id;
            model[Id / 2] = (Id ^ 1) % 2;
            level[Id / 2] = dlevel;
            reason[Id / 2] = static_cast<int>(clauses.size()) - 1;
            dstack.push_back(Id);
            continue;
        }
        int result = evaluateCNF(clauses, model);

        if (result == true)
            return true;

        int decisionId = Decide(model);
        model[decisionId] = TRUE;
        dlevel++;
        dstacklevel.push_back(dstack.size());
        dstack.push_back(2 * decisionId);
        reason[decisionId] = -1;
        level[decisionId] = dlevel;
    }
}

bool CDCL(Formula formula)
{
    std::pair<CNF, std::vector<std::string>> TseitinEncoded = TseitinEncodedCNF(formula);
    return CDCLCNF(TseitinEncoded.first);
}

bool CDCLCNF(const CNF &F)
{
    std::set<Clause> set_clauses = F.getClauses();
    Clauses clauses = std::vector<Clause>(set_clauses.begin(), set_clauses.end());
    return CDCLClauses(clauses, F.propNums());
}
