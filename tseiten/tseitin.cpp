
#include "tseitin.h"

void TseitinRecursive(Node *root, CNF &tseitin)
{
    if (!root || (!root->left && !root->right))
        return;
    TseitinRecursive(root->left, tseitin);
    TseitinRecursive(root->right, tseitin);
    const int a = root->aux_id;
    const int p = root->left->aux_id;
    const char op = root->nodeString[0];
    switch (op)
    {
    case '|':
    {
        const int q = root->right->aux_id;
        tseitin.addClause({a ^ 1, p, q});
        tseitin.addClause({a, q ^ 1});
        tseitin.addClause({a, p ^ 1});
        break;
    }
    case '&':
    {
        const int q = root->right->aux_id;
        tseitin.addClause({a ^ 1, p});
        tseitin.addClause({a ^ 1, q});
        tseitin.addClause({a, p ^ 1, q ^ 1});
        break;
    }
    case '>':
    {
        const int q = root->right->aux_id;
        tseitin.addClause({a ^ 1, p ^ 1, q});
        tseitin.addClause({a, p});
        tseitin.addClause({a, q ^ 1});
        break;
    }
    case '^':
    {
        const int q = root->right->aux_id;
        tseitin.addClause({a ^ 1, p ^ 1, q ^ 1});
        tseitin.addClause({a ^ 1, p, q});
        tseitin.addClause({a, p, q ^ 1});
        tseitin.addClause({a, p ^ 1, q});
        break;
    }
    case '~':
        tseitin.addClause({a ^ 1, p ^ 1});
        tseitin.addClause({a, p});
        break;
    }
}

// This is the function which recurses without the root in its clauses.
// It basically checks over the operator at the root and adds the required clause and recurses deeper into the tree.
// At last merged with the smaller sub CNF formulas.

CNF TseitinWithoutRootRecursive(Node *root)
{
    CNF tseitin;
    TseitinRecursive(root, tseitin);
    return tseitin;
}

// This function is called in the beginning of TseitinEncodedCNF.
// It basically takes the root,Proplookup and adds the new aux variables in the map
//   corresponding them to their new ids(aux_ids)(which we generate in AssignAuxId function).

// void AuxPropLookup(Node *root, std::vector<std::string> &propIdMap)
// {
//     if (!root)
//         return;
//     // if leaf also we dont need to make a new aux var
//     if (!root->left && !root->right)
//         return;
//     AuxPropLookup(root->left, propIdMap);
//     AuxPropLookup(root->right, propIdMap);
//     std::string auxvar = std::to_string(root->aux_id);
//     propIdMap[auxvar] = root->aux_id;
//     return;
// }
// This function is called in the beginning of TseitinEncodedCNF.
// It basically takes the root,Idlookup and adds the new aux_id in the map
//   corresponding them to their aux_vars(which we generate in AssignAuxId function).

// assigns aux_ids(increasing from bottom to top) to internal nodes and for atomic props assigns aux_id as the id thats already given for it in lookup
void AssignAuxId(Node *root, std::vector<std::string> &propIdMap)
{
    if (!root)
        return;
    if (!root->left && !root->right)
    {
        root->aux_id = root->id;
        return;
    }
    AssignAuxId(root->left, propIdMap);
    AssignAuxId(root->right, propIdMap);
    int varIdx = static_cast<int>(propIdMap.size()) + 1;
    root->aux_id = varIdx << 1;
    std::string name = "a";
    name += std::to_string(varIdx);
    propIdMap.push_back(std::move(name));
    return;
}

// This function basically gives us the req Tseiten encoded CNF formula
//   with its PropLookup and IdLookup containing the aux vars, their ids.
// It works on the principle of recursion first we separate the root,
//   so that we can recurse we get that cnf from the next func.
// Here we invoke some functions at the start which basically assign aux ids to every internal node in the tree
//   and also build up our final cnf's lookup maps.
// At last we insert the root unit clause and return.
std::pair<CNF, std::vector<std::string>> TseitinEncodedCNF(Formula formula)
{

    Node *root = formula.root;
    if (!root)
        return {CNF(), {}};
    std::vector<std::string> propIdMap = std::move(formula.propIdMap);
    AssignAuxId(root, propIdMap);
    CNF clauses = TseitinWithoutRootRecursive(root);
    clauses.addClause({root->aux_id});
    return {std::move(clauses), std::move(propIdMap)};
}
