#include "evaluator/evaluator.h"
#include "cnf/converting_functions.h"
#include "validity/validity.h"

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
    std::cout << "===========Current Assignment=============" << "\n";
    a.printModel();
    std::cout << "=========Formula evaluation===========" << "\n";
    std::cout << "F: " << f.evaluateFormula(a, treeNode) << std::endl;
    std::cout << "String: " << treeToStr(treeNode) << std::endl;
    Node *UNI_NODE = UNI_ONLY(treeNode);
    std::cout << "Universal String: " << treeToStr(UNI_NODE) << std::endl;
    Node *NNF_NODE = NNF(UNI_NODE);
    std::cout << "NNF String: " << treeToStr(NNF_NODE) << std::endl;
    std::cout << "CNF String: ";
    CNF Formula_in_CNF = convertToCNF(NNF_NODE);
    Formula_in_CNF.print();
    std::cout << "Validity: " << isCNFValid(Formula_in_CNF) << "\n";
}