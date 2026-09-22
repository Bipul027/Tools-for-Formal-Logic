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

bool CNF::empty() const
{
    return clauses.empty();
}

int CNF::size() const
{
    return clauses.size();
}

void CNF::print(const std::vector<std::string> &propIdMap)
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
            std::cout << propIdMap.at(abs(id) - 1) << "|";
            n--;
            it++;
        }

        std::cout << (*it < 0 ? "" : "~");
        std::cout << propIdMap.at(abs(*it) - 1);
        std::cout << ")";

        ct++;
        if (ct < (int)clauses.size())
            std::cout << "&";
    }
    std::cout << '\n';
}

void CNF::merge(const CNF &other)
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

std::set<std::set<int>> CNF::CNFtoTree() const
{
    return clauses;
}
std::set<std::set<int>> CNF::addClause(std::set<int> &clause)
{
    clauses.insert(clause);
}