#include "cnf.h"

// CNF = {{} & {}... & {}}

// Initialises an empty CNF
CNF::CNF()
{
    this->clauses = std::set<std::set<int>>();
}

// Initialises a CNF with a given set of clauses
CNF::CNF(std::set<std::set<int>> &clauses)
{
    this->clauses = clauses;
}

// Initialises a CNF with exactly one clause
CNF::CNF(std::set<int> &clause)
{
    clauses.insert(clause);
}

// Initialises a CNF with exactly one variable i.e. CNF := {{p}}
CNF::CNF(int id)
{
    clauses.insert({id});
}

bool CNF::empty()
{
    return clauses.empty();
}

int CNF::size()
{
    return clauses.size();
}

void CNF::print(const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id)
{
    int ct = 0;
    for (auto &clause : clauses)
    {
        std::cout << "(";

        int n = clause.size();
        auto it = clause.begin();
        while (n > 1)
        {
            int id = *it;
            std::cout << (id < 0 ? "" : "~");
            std::cout << id_to_prop.at(abs(id)) << "|";
            n--;
            it++;
        }

        std::cout << (*it < 0 ? "" : "~");
        std::cout << id_to_prop.at(abs(*it));
        std::cout << ")";

        ct++;
        if (ct < (int)clauses.size())
            std::cout << "&";
    }
    std::cout << '\n';
}

void CNF::merge(CNF &other)
{
    for (auto &clause : other.clauses)
    {
        clauses.insert(clause);
    }
}

std::set<int> CNF::pop()
{
    auto it = clauses.begin();
    auto temp = *it;
    clauses.erase(it);

    return temp;
}

std::set<std::set<int>> CNF::CNFtoTree()
{
    std::set<std::set<int>> CNFSet;
    for (const auto &clause : clauses)
    {
        std::set<int> clauseSet(clause.begin(), clause.end());
        CNFSet.insert(clauseSet);
    }
    return CNFSet;
}