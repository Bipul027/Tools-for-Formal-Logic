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
    std::vector<std::set<int>> clauses;
    std::unordered_map<std::string, int> prop_to_id;
    std::unordered_map<int, std::string> id_to_prop;

public:
    CNF(const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id);
    CNF(std::set<int> &clause, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id);
    CNF(std::vector<std::set<int>> &clauses, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id);
    CNF(std::string &prop, bool state, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id);
    bool empty();
    int size();
    void print();
    void merge(CNF &other);
    std::set<int> pop();
    std::vector<std::set<int>> CNFtoTree();
};