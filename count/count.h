#include "../wff/validator.h"
#include "../tseiten/tseitin.h"
#include "../cdcl/vsids.h"
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

Result Count(const Formula &formula);
Result CountCNF(const CNF &F);