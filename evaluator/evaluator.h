#pragma once
#include "../wff/validator.h"

struct assignment
{
    std::map<std::string, bool> model;
    assignment() {}
    assignment(std::map<std::string, bool> &model) : model(model) {}

    void buildModel(Node *root);
    void printModel();
};
