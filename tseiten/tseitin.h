#include "../cnf/cnf.h"
#include "../wff/validator.h"
#include "../cnf/converting_functions.h"

std::pair<CNF, std::vector<std::string>> TseitinEncodedCNF(Formula formula);