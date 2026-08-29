#include "evaluator/evaluator.h"

int main()
{

    std::string s;
    std::getline(std::cin, s);
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    Formula f = Formula(s);
    Node *treeNode = f.buildParseTree(s);
    f.printTree();
    assignment a;
    a.buildModel(treeNode);
    a.printModel();
}