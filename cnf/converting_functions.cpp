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

Node *copyTree(Node *root)
{
    if (!root)
        return root;
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

    Node *copyF = copyTree(F);
    Node *copyG = copyTree(G);

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

CNF merge(CNF F, CNF G)
{
    CNF temp;
    temp.merge(F);
    temp.merge(G);

    return temp;
}

// Assumes that F and G are in CNF
// DISTR (F , G) computes a CNF for F | G
CNF DISTR(CNF F, CNF G)
{
    if (G.empty())
        return F;
    if (F.empty())
        return G;

    int n = F.size(), m = G.size();

    if (n >= 2)
    {
        auto temp = F.pop();
        CNF F0 = CNF(temp);

        return merge(DISTR(F0, G), DISTR(F, G));
    }

    if (m >= 2)
    {
        auto temp = G.pop();
        CNF G0 = CNF(temp);

        return merge(DISTR(F, G0), DISTR(F, G));
    }

    auto tmp1 = F.pop(), tmp2 = G.pop();

    for (auto literal : tmp2)
    {
        tmp1.push_back(literal);
    }

    return CNF(tmp1);
}

// Assumes formula already in NNF
CNF convertToCNF(Node *root)
{
    if (!root)
    {
        return CNF();
    }

    if (!(root->left) && !(root->right))
    {
        return CNF({root->nodeString, true});
    }

    if (root->nodeString == "~")
    {
        return CNF({root->left->nodeString, false});
    }

    if (root->nodeString == "&")
    {
        return merge(convertToCNF(root->left), convertToCNF(root->right));
    }

    if (root->nodeString == "|")
    {
        return DISTR(convertToCNF(root->left), convertToCNF(root->right));
    }

    return CNF();
}