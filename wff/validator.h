#pragma once
#include <bits/stdc++.h>

struct Node
{
    std::string nodeString;
    Node *left, *right;

    Node() : left(nullptr), right(nullptr) {}
    Node(std::string str) : nodeString(str), left(nullptr), right(nullptr) {}
    Node(std::string str, Node *l, Node *r) : nodeString(str), left(l), right(r) {}

    ~Node()
    {
        delete left;
        delete right;
        nodeString = "";
    }
    bool isNodeValid();
};

class Formula
{
private:
    Node *root;
    bool isUnaryOperator(char x);
    bool isBinaryOperator(char x);
    void printParseTree(Node *root);

public:
    Formula() : root(nullptr) {}
    Formula(Node *root) : root(root) {}

    Node *buildParseTree(std::string &formulaString);

    Formula(std::string s)
    {
        root = buildParseTree(s);
    }

    void printTree();
};