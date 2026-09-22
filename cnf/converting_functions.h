#pragma once
#include "../wff/validator.h"
#include "cnf.h"

Node *IMPL_FREE(Node *root);
Node *XOR_FREE(Node *root);
Node *UNI_ONLY(Node *root);
Node *NNF(Node *root);
CNF merge(const CNF &F,const CNF &G);
CNF DISTR(const CNF &F,const CNF &G);
CNF convertToCNF(Node *root);