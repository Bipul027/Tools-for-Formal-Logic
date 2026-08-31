#pragma once
#include "../wff/validator.h"
#include "cnf.h"

Node *IMPL_FREE(Node *root);
Node *XOR_FREE(Node *root);
Node *UNI_ONLY(Node *root);
Node *NNF(Node *root);
CNF merge(CNF F, CNF G);
CNF DISTR(CNF F, CNF G);
CNF convertToCNF(Node* root);