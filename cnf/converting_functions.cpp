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
        return nullptr;

    Node *copy = new Node(
        root->nodeString,
        copyTree(root->left),
        copyTree(root->right));

    copy->id = root->id;

    return copy;
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

CNF merge(const CNF &F, const CNF &G)
{
    CNF temp;
    temp.merge(F);
    temp.merge(G);
    return temp;
}

// Assumes that F and G are in CNF
// DISTR (F , G) computes a CNF for F | G
CNF DISTR(const CNF &F, const CNF &G)
{
    if (G.empty())
        return F;
    if (F.empty())
        return G;

    CNF result;

    for (const auto &c1 : F.CNFtoTree())
    {
        for (const auto &c2 : G.CNFtoTree())
        {
            std::set<int> clause = c1;

            clause.insert(c2.begin(), c2.end());

            result.addClause(clause);
        }
    }

    return result;
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
        return CNF(root->id);
    }

    if (root->nodeString == "~")
    {
        return CNF(-(root->left->id));
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