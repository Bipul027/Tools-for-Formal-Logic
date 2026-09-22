#include "evaluator.h"

/**
 * Convert every proposition into an integer id instead of storing them as strings
 */

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

void assignment::makeId(Node *root)
{
    if (model.empty())
        buildModel(root);
    /**
        
    */
}

void assignment::printModel()
{
    for (auto &[prop, value] : model)
    {
        std::cout << prop << " : " << value << "\n";
    }
}
