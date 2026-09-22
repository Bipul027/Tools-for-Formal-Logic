#pragma once
#include <iostream>
#include <set>
#include <map>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct assignment;

struct Node
{
    // To remove the storage of the string and rely on id completely.
    std::string nodeString;
    // Operations have an id 0 irrespective of their nature and every proposition has a unique id.
    int id = 0;
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

std::string treeToStr(Node *root);

class Formula
{
private:
    bool isUnaryOperator(char x);
    bool isBinaryOperator(char x);
    void printParseTree(Node *root);

public:
    Node *root;
    int propNums = 0;
    std::unordered_map<std::string, int> propLookupMap;
    std::unordered_map<int, std::string> propIdMap;

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