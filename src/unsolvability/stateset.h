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
    int size;
public:
    StateSet(std::string name, BDD states, int size);
    bool contains(const Cube &state);
    std::string getName();
    int getSize();
};

#endif // STATESET_H
