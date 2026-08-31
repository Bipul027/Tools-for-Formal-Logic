#include "validity.h"

bool isCNFValid(CNF Formula)
{
    std::set<std::set<std::pair<std::string, bool>>> CNFTree = Formula.CNFtoTree();

    bool isValid = true;
    for (auto clause : CNFTree)
    {
        bool isclauseValid = false;
        for (auto literal : clause)
        {
            if (clause.find({literal.first, !literal.second}) != clause.end())
            {
                isclauseValid = true;
                break;
            }
        }
        if (!isclauseValid)
        {
            isValid = false;
            break;
        }
    }
    return isValid;
}
bool isFormulaValid(std::string FormulaString)
{
    Formula formula;
    Node *root = formula.buildParseTree(FormulaString);
    CNF CNFForm = convertToCNF(NNF(UNI_ONLY(root)));
    return isCNFValid(CNFForm);
}