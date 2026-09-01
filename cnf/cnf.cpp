#include "cnf.h"

// CNF = {{} & {}... & {}}
CNF::CNF()
{
    clauses = std::set<std::set<std::pair<std::string, bool>>>();
}

CNF::CNF(std::set<std::set<std::pair<std::string, bool>>> &clauses)
{
    CNF();
    this->clauses = clauses;
}

CNF::CNF(std::set<std::pair<std::string, bool>> &clause)
{
    CNF();
    clauses.insert(clause);
}

CNF::CNF(std::string &prop, bool state)
{
    clauses.insert({{prop, state}});
}

bool CNF::empty()
{
    return clauses.empty();
}

int CNF::size()
{
    return clauses.size();
}

void CNF::print()
{
    int ct = 0;
    for (auto &clause : clauses)
    {
        std::cout << "(";

        int n = clause.size();
        auto it = clause.begin();
        while(n > 1)
        {
            auto [prop, state] = *it;
            std::cout << (state ? "" : "~");
            std::cout << prop << "|";
            n--;
            it++;
        }

        std::cout << (it->second ? "" : "~");
        std::cout << it->first;
        std::cout << ")";

        ct++;
        if (ct < (int)clauses.size())
            std::cout << "&";
    }
    std::cout << '\n';
}

void CNF::merge(CNF &other)
{
    for (auto clause : other.clauses)
    {
        clauses.insert(clause);
    }
}

std::set<std::pair<std::string, bool>> CNF::pop()
{
    auto it = clauses.begin();
    auto temp = *it;
    clauses.erase(it);

    return temp;
}

std::set<std::set<std::pair<std::string, bool>>> CNF::CNFtoTree()
{
    std::set<std::set<std::pair<std::string, bool>>> CNFSet;
    for (const auto &clause : clauses)
    {
        std::set<std::pair<std::string, bool>> clauseSet(clause.begin(), clause.end());
        CNFSet.insert(clauseSet);
    }
    return CNFSet;
}