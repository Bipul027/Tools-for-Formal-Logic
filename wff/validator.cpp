#include "validator.h"

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
 * F ::= (p) or (~p) or (F | F) or (F & F) or (F ^ F) or (F > F) or ~(F)
 */
Node *Formula::buildParseTree(std::string &formulaString)
{
    int n = formulaString.size();

    /**
     * If the string starts with ~ then F ::= ~(G), hence we recursively build the parsetree of G and attach it as a leftChild for '~'
     */
    if (formulaString[0] == '~')
    {
        std::string str = formulaString.substr(2, n - 3);
        Node *l = buildParseTree(str);

        return new Node("~", l, nullptr);
    }
    /**
     * If the string starts with ~ the F ::= (p) or F ::= (G op H) or (~p)
     */
    if (formulaString[0] == '(')
    {
        bool noOperatorInString = true;
        for (char x : formulaString)
        {
            if (isBinaryOperator(x))
                noOperatorInString = false;
        }

        if (noOperatorInString)
        {
            return new Node(formulaString);
        }

        int count = 0, indexOfOperator = -1;
        for (int i = 1; i < n; i++)
        {
            if (formulaString[i] == '(')
                count++;
            else if (formulaString[i] == ')')
                count--;

            if (count == 0 && isBinaryOperator(formulaString[i]))
            {
                indexOfOperator = i;
                break;
            }
        }
        if (indexOfOperator != -1)
        {
            std::string leftStr = formulaString.substr(1, indexOfOperator - 1);
            std::string rightStr = formulaString.substr(indexOfOperator + 1, n - indexOfOperator - 2);
            Node *l = buildParseTree(leftStr);
            Node *r = buildParseTree(rightStr);

            std::string op(1, formulaString[indexOfOperator]);
            return new Node(op, l, r);
        }
    }

    return new Node(formulaString);
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

int main()
{
    std::string s;
    std::getline(std::cin, s);

    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    Formula f = Formula(s);
    f.printTree();
}