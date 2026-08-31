#pragma once
#include <iostream>
#include <bits/stdc++.h>


class CNF
{
private:
    std::vector<std::vector<std::pair<std::string, bool>>> clauses;

public:
    CNF();
    CNF(std::vector<std::pair<std::string, bool>> &clause);
    CNF(std::vector<std::vector<std::pair<std::string, bool>>> &clauses);
    CNF(std::string &prop, bool state);

    bool empty();
    int size();
    void print();
    void merge(CNF &other);
    std::vector<std::pair<std::string, bool>> pop();
    std::set<std::set<std::pair<std::string, bool>>> CNFtoTree();
};