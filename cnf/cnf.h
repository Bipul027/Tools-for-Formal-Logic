#pragma once
#include <iostream>
#include <set>
#include<set>
#include<algorithm>
#include<string>
#include<map>

class CNF
{
private:
    std::set<std::set<std::pair<std::string, bool>>> clauses;

public:
    CNF();
    CNF(std::set<std::pair<std::string, bool>> &clause);
    CNF(std::set<std::set<std::pair<std::string, bool>>> &clauses);
    CNF(std::string &prop, bool state);

    bool empty();
    int size();
    void print();
    void merge(CNF &other);
    std::set<std::pair<std::string, bool>> pop();
    std::set<std::set<std::pair<std::string, bool>>> CNFtoTree();
};