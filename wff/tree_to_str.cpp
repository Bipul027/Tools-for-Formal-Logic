#include "validator.h"

std::string treeToStr(Node *root)
{
    if (!root) return "";
    
    if (!(root->left) && !(root->right))
    {
        return root->nodeString;
    }

    if (root->nodeString == "~")
    {
        return "~" + treeToStr(root->left);
    }

    return "(" + treeToStr(root->left) + (root->nodeString) + treeToStr(root->right) + ")";
}