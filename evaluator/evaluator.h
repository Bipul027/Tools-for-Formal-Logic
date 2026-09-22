#pragma once
#include "../wff/validator.h"

struct assignment
{

    std::map<std::string, bool> model;
    std::map<int, std::string> idToProp;
    assignment() {}
    assignment(std::map<std::string, bool> &model) : model(model) {}

    void buildModel(Node *root);
    void makeId(Node *root);
    void printModel();
};
