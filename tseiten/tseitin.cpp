#pragma once
#include "../cnf/cnf.h"

//This function basically gives us the req Tseiten encoded CNF formula
//  with its PropLookup and IdLookup containing the aux vars, their ids.
//It works on the principle of recursion first we separate the root,
//  so that we can recurse we get that cnf from the next func.
//Here we invoke some functions at the start which basically assign aux ids to every internal node in the tree
//  and also build up our final cnf's lookup maps.
//At last we insert the root unit clause and return.  
CNF TseitinEncodedCNF(Formula f){
    CNF tseitin = CNF();
    Node* root = f.root;
    if(!root) return tseitin;
    tseitin.prop_to_id = f.propLookup;
    tseitin.id_to_prop = f.PropIdMap;
    int nextid = f.PropNums + 1;
    AssignAuxId(root,nextid,f.propLookup);
    AuxPropLookup(root,tseitin.prop_to_id);
    AuxIdLookup(root,tseitin.id_to_prop);
    CNF clauses = TseitinWithoutRootRecursive(root);
    clauses.insert({root->aux_id});
    clauses.prop_to_id = tseitin.prop_to_id;
    clauses.id_to_prop = tseitin.id_to_prop;
    return clauses;
}

//This is the function which recurses without the root in its clauses.
//It basically checks over the operator at the root and adds the required clause and recurses deeper into the tree.
//At last merged with the smaller sub CNF formulas.
CNF TseitinWithoutRootRecursive(Node* root){
    CNF tseitin = CNF();
    if(!root) return tseitin;
    if(!root->left && !root->right) return tseitin;
    CNF tseitin_left = TseitinWithoutRootRecursive(root->left);
    CNF tseitin_right = TseitinWithoutRootRecursive(root->right);    
    if(root->nodeString=="|"){
        //add clauses (~a|p|q),(a|~p),(a|~q)
        int a = root->aux_id;
        int p = root->left->aux_id;
        int q = root->right->aux_id;
        tseitin.insert({0-a,p,q});
        tseitin.insert({a,0-q});
        tseitin.insert({a,0-p});
    }
    else if(root->nodeString=="&"){
        //add clauses (~a|p),(~a|q),(a|~p|~q)
        int a = root->aux_id;
        int p = root->left->aux_id;
        int q = root->right->aux_id;
        tseitin.insert({0-a,p});
        tseitin.insert({0-a,q});
        tseitin.insert({a,0-p,0-q});
    }
    else if(root->nodeString==">"){
        //add clauses (~a|~p|q),(a|p),(a|~q)
        int a = root->aux_id;
        int p = root->left->aux_id;
        int q = root->right->aux_id;
        tseitin.insert({0-a,0-p,q});
        tseitin.insert({a,p});
        tseitin.insert({a,0-q});
    }
    else if(root->nodeString=="^"){
        //add clauses (~a|p|q),(~a|~p|~q),(a|p|~q),(a|~p|q)
        int a = root->aux_id;
        int p = root->left->aux_id;
        int q = root->right->aux_id;
        tseitin.insert({0-a,0-p,0-q});
        tseitin.insert({0-a,p,q});
        tseitin.insert({a,p,0-q});
        tseitin.insert({a,0-p,q});
    }
    CNF temp = merge(tseitin_left,tseitin_right);
    tseitin = merge(tseitin,temp);
    return tseitin;
}

//This function is called in the beginning of TseitinEncodedCNF.
//It basically takes the root,Proplookup and adds the new aux variables in the map 
//  corresponding them to their new ids(aux_ids)(which we generate in AssignAuxId function).
void AuxPropLookup(Node* root,unordered_map<std::string,int> &lookup){
    if(!root) return;
    //if leaf also we dont need to make a new aux var
    if(!root->left && !root->right) return;
    AuxPropLookup(root->left,lookup);
    AuxPropLookup(root->right,lookup);
    std::string auxvar = "a_" + std::to_string(root->aux_id);
    lookup[auxvar] = root->aux_id; 
    return;
}
//This function is called in the beginning of TseitinEncodedCNF.
//It basically takes the root,Idlookup and adds the new aux_id in the map 
//  corresponding them to their aux_vars(which we generate in AssignAuxId function).
void AuxIdLookup(Node* root,unordered_map<int,std::string> &Idlookup){
    if(!root) return;
    if(!root->left && !root->right) return;
    AuxIdLookup(root->left,Idlookup);
    AuxIdLookup(root->right,Idlookup);
    Idlookup[root->aux_id]="a_"+std::to_string(root->aux_id);
    return;
}
//assigns aux_ids(increasing from bottom to top) to internal nodes and for atomic props assigns aux_id as the id thats already given for it in lookup
void AssignAuxId(Node* root,int& nextid,unordered_map<std::string,int>& lookup){
    if(!root) return;
    if(!root->left&&!root->right) {root->aux_id = lookup[root->nodeString];return;}
    AssignAuxId(root->left,nextid,lookup);
    AssignAuxId(root->right,nextid,lookup);
    root->aux_id = nextid;
    nextid++;
    return;
}
