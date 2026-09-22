#pragma once
#include "../wff/validator.h"
#include "cnf.h"

Node *IMPL_FREE(Node *root);
Node *XOR_FREE(Node *root);
Node *UNI_ONLY(Node *root);
Node *NNF(Node *root);
CNF merge(CNF F, CNF G, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id);
CNF DISTR(CNF F, CNF G, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id);
CNF convertToCNF(Node* root, const std::unordered_map<int, std::string> &id_to_prop, const std::unordered_map<std::string, int> &prop_to_id);