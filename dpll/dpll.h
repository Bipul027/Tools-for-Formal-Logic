#include "../wff/validator.h"
#include "../tseiten/tseitin.h"

bool DPLL(Formula formula);
bool DPLLCNF(const CNF &F);
bool DPLLCNF(const CNF &F, std::unordered_map<int, int> &part_assign);