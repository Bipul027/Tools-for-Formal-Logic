#include "validator.h"
#include "../evaluator/evaluator.h"
/**
 * struct Node{
 *      Node *left, *right;
 *      string nodeString;
 * }
 */
bool Node::isNodeValid()
{
    if (nodeString == "~")
    {
        if (right)
            return false;
    }
    if (nodeString != "~" && nodeString != "&" && nodeString != "|" && nodeString != "^" && nodeString != ">")
    {
        if (right || left)
            return false;
    }
    return true;
}

bool Formula::isUnaryOperator(char x)
{
    if (x == '~')
        return true;
    return false;
}

bool Formula::isBinaryOperator(char x)
{
    if (x == '&' || x == '^' || x == '|' || x == '>')
        return true;
    return false;
}

/**
 * Syntax for the Formula (Strict) in Backus-Naur Form
 * F ::= p or ~(p) or (F | F) or (F & F) or (F ^ F) or (F > F) or ~(F)
 */

Node *Formula::parse(const std::string &s, size_t &pos)
{
    if (s[pos] == '~')
    {
        pos++;
        return new Node("~", parse(s, pos), nullptr);
    }
    if (s[pos] == '(')
    {
        pos++;
        Node *left = parse(s, pos);
        if (s[pos] == ')')
        {
            ++pos;
            return left;
        }
        const char op = s[pos++];
        Node *right = parse(s, pos);
        pos++;
        return new Node(std::string(1, op), left, right);
    }
    // base case
    const size_t start = pos;
    while (pos < s.size() && s[pos] != '(' && s[pos] != ')' && !isBinaryOperator(s[pos]) && s[pos] != '~')
        pos++;
    std::string name = s.substr(start, pos - start);
    auto [it, inserted] = propToId.try_emplace(name, 0);
    if (inserted)
    {
        propIdMap.push_back(name);
        it->second = static_cast<int>(propIdMap.size());
    }
    Node *leaf = new Node(name);
    leaf->id = 2 * it->second;
    return leaf;
}

Node *Formula::buildParseTree(std::string &formulaString)
{
    size_t pos = 0;
    return parse(formulaString, pos);
}

void Formula::printParseTree(Node *root)
{
    if (!root)
    {
        std::cout << "";
        return;
    }

    printParseTree(root->left);
    std::cout << " : " << root->nodeString << " : ";

    printParseTree(root->right);
}

void Formula::printTree()
{
    printParseTree(root);
    std::cout << '\n';
}

bool Formula::evaluateFormula(assignment &assign, Node *root)
{
    std::map<std::string, bool> &model = assign.model;
    if (!isUnaryOperator((root->nodeString)[0]) && !isBinaryOperator((root->nodeString)[0]))
        return model[root->nodeString];

    if (isUnaryOperator((root->nodeString)[0]) && root->nodeString.length() == 1)
        return !evaluateFormula(assign, root->left);

    if (root->nodeString == "|")
        return (evaluateFormula(assign, root->left) || evaluateFormula(assign, root->right));

    if (root->nodeString == "&")
        return (evaluateFormula(assign, root->left) && evaluateFormula(assign, root->right));

    if (root->nodeString == "^")
        return (evaluateFormula(assign, root->left) != evaluateFormula(assign, root->right));

    if (root->nodeString == ">")
        return (!evaluateFormula(assign, root->left) || evaluateFormula(assign, root->right));
    return true;
};