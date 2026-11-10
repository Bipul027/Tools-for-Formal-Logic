#include "cnf.h"

// CNF = {{} & {}... & {}}

// Initialises an empty CNF
CNF::CNF() = default;

// Initialises a CNF with a given set of clauses
CNF::CNF(std::set<std::set<int>> &clauses) : clauses(std::move(clauses))
{
}

// Initialises a CNF with exactly one clause
CNF::CNF(std::set<int> &clause)
{
    clauses.insert(std::move(clause));
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

    for (const auto &clause : clauses)
    {
        std::cout << "(";

        int n = static_cast<int>(clause.size());
        auto it = clause.begin();

        while (n > 1)
        {
            int literal = *it;

            bool negated = literal & 1;
            int idx = literal >> 1;

            if (negated)
                std::cout << "~";

            std::cout << propIdMap.at(idx - 1) << "|";

            n--;
            it++;
        }

        int literal = *it;

        bool negated = literal & 1;
        int idx = literal >> 1;

        if (negated)
            std::cout << "~";

        std::cout << propIdMap.at(idx - 1);

        std::cout << ")";

        ++ct;

        if (ct < static_cast<int>(clauses.size()))
            std::cout << "&";
    }

    std::cout << '\n';
}

void CNF::merge(const CNF &other)
{
    clauses.insert(other.clauses.begin(), other.clauses.end());
}

std::set<int> CNF::pop()
{
    return std::move(clauses.extract(clauses.begin()).value());
}

std::set<std::set<int>> CNF::CNFtoTree() const
{
    return clauses;
}
void CNF::addClause(std::set<int> clause)
{
    clauses.insert(std::move(clause));
}
const std::set<std::set<int>> &CNF::getClauses() const
{
    return clauses;
}

int CNF::propNums() const
{
    int maxElement = 0;
    for (const auto &clause : clauses)
    {
        if (!clause.empty())
        {
            maxElement = std::max(maxElement, *clause.rbegin());
        }
    }
    return maxElement >> 1;
}