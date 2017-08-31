#ifndef STATESET_H
#define STATESET_H

#include "task.h"

#include "cuddObj.hh"

class StateSet
{
private:
    static Cudd manager;
    std::string name;
    BDD states;
public:
    StateSet(std::string name, BDD states);
    bool contains(const Cube &state);
    std::string getName();
};

#endif // STATESET_H
