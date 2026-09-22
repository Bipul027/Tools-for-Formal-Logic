#include "cnf.h"

// CNF = {{} & {}... & {}}

// Initialises an empty CNF
CNF::CNF(const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id)
{
    this->prop_to_id = prop_to_id;
    this->id_to_prop = id_to_prop;
    this->clauses = std::vector<std::set<int>>();
}

// Initialises a CNF with a given set of clauses
CNF::CNF(std::vector<std::set<int>> &clauses, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id)
{
    CNF(id_to_prop, prop_to_id);

    this->prop_to_id = prop_to_id;
    this->id_to_prop = id_to_prop;
    this->clauses = clauses;
}

// Initialises a CNF with exactly one clause
CNF::CNF(std::set<int> &clause, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id)
{
    CNF(id_to_prop, prop_to_id);
    this->prop_to_id = prop_to_id;
    this->id_to_prop = id_to_prop;
    clauses.push_back(clause);
}

// Initialises a CNF with exactly one variable i.e. CNF := {{p}}
CNF::CNF(std::string &prop, bool state, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id)
{
    this->prop_to_id = prop_to_id;
    this->id_to_prop = id_to_prop;
    int propId = prop_to_id.at(prop);
    int mul = state ? 1 : -1;
    clauses.push_back({mul * propId});
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
        while (n > 1)
        {
            int id = *it;
            std::cout << (id < 0 ? "" : "~");
            std::cout << id_to_prop[abs(id)] << "|";
            n--;
            it++;
        }

        std::cout << (*it < 0 ? "" : "~");
        std::cout << id_to_prop[abs(*it)];
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
        clauses.push_back(clause);
    }
}

std::set<int> CNF::pop()
{
    auto it = clauses.begin();
    auto temp = *it;
    clauses.erase(it);

    return temp;
}

std::vector<std::set<int>> CNF::CNFtoTree()
{
    std::vector<std::set<int>> CNFSet;
    for (const auto &clause : clauses)
    {
        std::set<int> clauseSet(clause.begin(), clause.end());
        CNFSet.push_back(clauseSet);
    }
    return CNFSet;
}