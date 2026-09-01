#pragma once
#include <iostream>
#include <set>
#include<set>
#include<algorithm>
#include<string>
#include<map>

struct assignment;

struct Node
{
    std::string nodeString;
    Node *left, *right;

    Node() : left(nullptr), right(nullptr) {}
    Node(std::string str) : nodeString(str), left(nullptr), right(nullptr) {}
    Node(std::string str, Node *l, Node *r) : nodeString(str), left(l), right(r) {}

    ~Node()
    {
        nodeString = "";
    }
    bool isNodeValid();
};

std::string treeToStr(Node* root);

class Formula
{
private:
    bool isUnaryOperator(char x);
    bool isBinaryOperator(char x);
    void printParseTree(Node *root);

public:
    Node *root;

    Formula() : root(nullptr) {}
    Formula(Node *root) : root(root) {}
    Formula(std::string s)
    {
        root = buildParseTree(s);
    }

    Node *buildParseTree(std::string &formulaString);

    void printTree();

    bool evaluateFormula(assignment &assign, Node *root);
};