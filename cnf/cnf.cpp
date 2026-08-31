#include "cnf.h"

CNF::CNF() {
    clauses = std::vector<std::vector<std::pair<std::string, bool>>>();
}

CNF::CNF(std::vector<std::vector<std::pair<std::string, bool>>> &clauses) {
    CNF();
    this->clauses = clauses;
}

CNF::CNF(std::vector<std::pair<std::string, bool>>& clause) {
    CNF();
    clauses.push_back(clause);
}

CNF::CNF(std::string &prop, bool state) {
    clauses.push_back({{prop, state}});
}

bool CNF::empty() {
    return clauses.empty();
}

int CNF::size() {
    return clauses.size();
}

void CNF::print() {
    int ct = 0;
    for (auto &clause : clauses) {
        std::cout << "( ";

        int n = clause.size();
        for (int i = 0; i < n - 1; i++) {
            auto [prop, state] = clause[i];
            std::cout << state? "" : "~";
            std::cout << prop << " & ";
        }

        std::cout << clause.back().second? "" : "~";
        std::cout << clause.back().first;
        std::cout << " )";

        ct++;
        if (ct < (int)clauses.size()) std::cout << " | ";
    }
    std::cout << '\n';
}

void CNF::merge(CNF &other) {
    for (auto clause : other.clauses) {
        clauses.push_back(clause);
    }
}

std::vector<std::pair<std::string, bool>> CNF::pop() {
    auto temp = clauses.back();
    clauses.pop_back();

    return temp;
}
