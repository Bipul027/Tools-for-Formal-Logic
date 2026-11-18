#include "../wff/validator.h"
#include "../tseiten/tseitin.h"

struct Result
{
    bool sat;
    std::vector<int> model;
    int conflicts;
    int decisions;
};

struct Watcher
{
    int clause_index;
    int blocker;
};

Result CDCL(const Formula &formula);
Result CDCLCNF(const CNF &F);