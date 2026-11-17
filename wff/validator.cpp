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
    //     int n = formulaString.size();

    //     /**
    //      * If the string starts with ~ then F ::= ~(G), hence we recursively build the parsetree of G and attach it as a leftChild for '~'
    //      */
    //     if (formulaString[0] == '~')
    //     {
    //         std::string str = formulaString.substr(1, n - 1);
    //         Node *l = buildParseTree(str);

    //         return new Node("~", l, nullptr);
    //     }
    //     /**
    //      * If the string starts with ( the  F ::= (G op H)
    //      */
    //     if (formulaString[0] == '(')
    //     {

    //         bool noOperatorInString = true;
    //         for (char x : formulaString)
    //         {
    //             if (isBinaryOperator(x))
    //                 noOperatorInString = false;
    //         }

    //         if (noOperatorInString)
    //         {
    //             return new Node(formulaString);
    //         }

    //         int count = 0, indexOfOperator = -1;
    //         for (int i = 1; i < n; i++)
    //         {
    //             if (formulaString[i] == '(')
    //                 count++;
    //             else if (formulaString[i] == ')')
    //                 count--;

    //             if (count == 0 && isBinaryOperator(formulaString[i]))
    //             {
    //                 indexOfOperator = i;
    //                 break;
    //             }
    //         }
    //         if (indexOfOperator != -1)
    //         {
    //             std::string leftStr = formulaString.substr(1, indexOfOperator - 1);
    //             std::string rightStr = formulaString.substr(indexOfOperator + 1, n - indexOfOperator - 2);
    //             Node *l = buildParseTree(leftStr);
    //             Node *r = buildParseTree(rightStr);

    //             std::string op(1, formulaString[indexOfOperator]);
    //             return new Node(op, l, r);
    //         }
    //     }

    //     // Base case when the string is just a proposition
    //     // Check if the given propositional variable already exists in prop and decide the id of the variable
    //     int idx = -1;

    //     for (int i = 0; i < static_cast<int>(propIdMap.size()); i++)
    //     {
    //         if (propIdMap[i] == formulaString)
    //         {
    //             idx = i + 1;
    //             break;
    //         }
    //     }

    //     if (idx == -1)
    //     {
    //         propIdMap.push_back(formulaString);
    //         idx = propIdMap.size();
    //     }

    //     Node *retNode = new Node(formulaString);

    //     retNode->id = 2 * idx;

    //     return retNode;
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