#include "converting_functions.h"

Node *IMPL_FREE(Node *root)
{
    if (!root)
        return root;

    if (!(root->left) && !(root->right))
    {
        return root;
    }

    if (root->nodeString != ">")
    {
        root->left = IMPL_FREE(root->left);
        root->right = IMPL_FREE(root->right);

        return root;
    }

    root->left = new Node("~", IMPL_FREE(root->left), nullptr);
    root->right = IMPL_FREE(root->right);
    root->nodeString = "|";

    return root;
}

Node* copyTree(Node *root) {
    if (!root) return root;
    return new Node(root->nodeString, copyTree(root->left), copyTree(root->right));
}

Node *XOR_FREE(Node *root)
{
    if (!root)
        return root;

    if (!(root->left) && !(root->right))
    {
        return root;
    }

    if (root->nodeString != "^")
    {
        root->left = XOR_FREE(root->left);
        root->right = XOR_FREE(root->right);

        return root;
    }

    // H = F ^ G
    // H = (F & ~G) | (~F & G)
    Node *F = XOR_FREE(root->left);
    Node *G = XOR_FREE(root->right);

    Node* copyF = copyTree(F);
    Node* copyG = copyTree(G);

    Node *notF = new Node("~", copyF, nullptr);
    Node *notG = new Node("~", copyG, nullptr);

    Node *left = new Node("&", F, notG);
    Node *right = new Node("&", notF, G);

    root->left = left;
    root->right = right;
    root->nodeString = "|";

    return root;
}

Node *UNI_ONLY(Node *root)
{
    return XOR_FREE(IMPL_FREE(root));
}

// Assumes implication free and xor free
Node *NNF(Node *root)
{
    if (!root)
        return root;

    if (!(root->left) && !(root->right))
        return root;

    if (root->nodeString == "~")
    {

        if (root->left->nodeString == "~")
        {
            Node *temp = root->left->left;
            delete root->left;
            delete root;
            return NNF(temp);
        }

        if (root->left->nodeString == "&")
        {
            root->right = new Node("~", root->left->right, nullptr);
            root->left->right = nullptr;

            root->left->nodeString = "~";
            root->nodeString = "|";
            
            root->left = NNF(root->left);
            root->right = NNF(root->right);

            return root;
        }

        if (root->left->nodeString == "|")
        {
            root->right = new Node("~", root->left->right, nullptr);
            root->left->right = nullptr;

            root->left->nodeString = "~";
            root->nodeString = "&";

            root->left = NNF(root->left);
            root->right = NNF(root->right);

            return root;
        }

        return root;
    }

    root->left = NNF(root->left);
    root->right = NNF(root->right);

    return root;
}