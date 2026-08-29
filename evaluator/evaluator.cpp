#include "evaluator.h"

void assignment::buildModel(Node *root)
{
    if (!(root->left) && !(root->right))
    {
        model[root->nodeString] = true;
        return;
    }
    if (!root->right)
    {
        buildModel(root->left);
        return;
    }
    buildModel(root->left);
    buildModel(root->right);
}

void assignment::printModel()
{
    for (auto &[prop, value] : model)
    {
        std::cout << prop << " : " << value << "\n";
    }
}

// int main()
// {
//     std::string s;
//     std::getline(std::cin, s);
//     s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
//     Formula f = Formula(s);
//     Node *treeNode = f.buildParseTree(s);
//     f.printTree();
//     assignment a;
//     a.buildModel(treeNode);
//     a.printModel();
// }