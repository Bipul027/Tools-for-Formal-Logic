#include "../wff/validator.h"
#include "../tseiten/tseitin.h"
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

struct Result
{
    bool sat;
    std::vector<int> model;
    int conflicts;
    int decisions;
    cpp_int satAssignments;
};

struct Watcher
{
    int clause_index;
    int blocker;
};

Result CDCL(const Formula &formula);
Result CDCLCNF(const CNF &F);
Result Count(const Formula &formula);
Result CountCNF(const CNF &F);