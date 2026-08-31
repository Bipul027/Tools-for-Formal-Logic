#include <iostream>
#include <vector>
#include <string>

class CNF {
private:
    std::vector<std::vector<std::pair<std::string, bool>>> clauses;

public:
    CNF();
    CNF(std::vector<std::pair<std::string, bool>>& clause);
    CNF(std::vector<std::vector<std::pair<std::string, bool>>> &clauses);
    CNF(std::string &prop, bool state);

    bool empty();
    int size();
    void print();
    void merge(CNF &other);
    std::vector<std::pair<std::string, bool>> pop();
};