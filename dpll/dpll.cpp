#include "dpll.h"

int evaluateClause(const std::set<int> &clause, const std::unordered_map<int, int> &part_assign)
{
    // return -1 if the clause is unassigned, 0 if false, 1 if true
    int res = 0;
    bool atleast_one_true = false;
    for (const int &varId : clause)
    {
        if (part_assign.at(varId / 2) == -1)
            res = -1;
        else if ((part_assign.at(varId / 2) ^ varId) % 2)
        {
            atleast_one_true = true;
            break;
        }
    }
    return (atleast_one_true ? 1 : res);
}

int evaluateCNF(const CNF &F, const std::unordered_map<int, int> &part_assign)
{
    std::set<std::set<int>> clauses = F.getClauses();
    for (const auto &clause : clauses)
    {
        int res = evaluateClause(clause, part_assign);
        if (res == 0)
            return 0;
        if (res == -1)
            return -1;
    }
    return 1;
}

bool UnitPropagate(const CNF &F, std::unordered_map<int, int> &part_assign)
{
    bool propagate = false;
    std::set<std::set<int>> clauses = F.getClauses();

    for (const auto &clause : clauses)
    {
        int idTopropagate = 0;
        int num_unassigned = 0;
        for (const auto &varId : clause)
        {
            if (part_assign[varId / 2] == -1)
            {
                num_unassigned++;
                idTopropagate = varId;
                continue;
            }
            if ((part_assign[varId / 2] ^ varId) % 2)
                break;
            else
                continue;
        }
        if (num_unassigned == 1)
        {
            part_assign[idTopropagate / 2] = (idTopropagate ^ 1) % 2;
            propagate = true;
        }
    }
    return propagate;
}

bool DPLL(Formula formula)
{
    Node *root = formula.root;
    std::pair<CNF, std::vector<std::string>> TseitinEncoded = TseitinEncodedCNF(formula);
    return DPLLCNF(TseitinEncoded.first);
}

bool DPLLCNF(const CNF &F)
{
    int propNums = F.propNums();
    std::unordered_map<int, int> part_assign;
    // -1 if a variable is unassigned, 0 means assigned to false, 1 means assigned to true
    for (int i = 1; i <= propNums; i++)
        part_assign[i] = -1;
    return DPLLCNF(F, part_assign);
}

bool DPLLCNF(const CNF &F, std::unordered_map<int, int> &part_assign)
{
    int evaluation = evaluateCNF(F, part_assign);

    if (evaluation == 1)
        return true;
    else if (evaluation == 0)
        return false;

    if (UnitPropagate(F, part_assign))
        return DPLLCNF(F, part_assign);

    int decisionId = 0;
    for (auto [key, val] : part_assign)
    {
        if (val == -1)
        {
            decisionId = key;
            break;
        }
    }
    part_assign[decisionId] = 0;

    if (DPLLCNF(F, part_assign))
        return true;
    part_assign[decisionId] = 1;

    return DPLLCNF(F, part_assign);
}
