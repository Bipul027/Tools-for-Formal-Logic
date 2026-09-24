#pragma once
#include <iostream>
#include <set>
#include <algorithm>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

class CNF
{
private:
    std::set<std::set<int>> clauses;
    // std::unordered_map<std::string, int> prop_to_id;
    // std::unordered_map<int, std::string> id_to_prop;

public:
    CNF();
    CNF(std::set<int> &clause);
    CNF(std::set<std::set<int>> &clauses);
    CNF(int id);
    bool empty() const;
    int size() const;
    void print(const std::vector<std::string> &propIdMap);
    void merge(const CNF &other);
    std::set<int> pop();
    std::set<std::set<int>> CNFtoTree() const;
    void addClause(std::set<int> clause);
    const std::set<std::set<int>> &getClauses() const;
    int propNums() const;
};