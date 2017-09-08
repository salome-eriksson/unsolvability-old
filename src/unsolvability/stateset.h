#ifndef STATESET_H
#define STATESET_H

#include "task.h"

#include "cuddObj.hh"

class StateSet
{
private:
    Cudd manager;
    std::string name;
    BDD states;
    int size;
public:
    StateSet(Cudd manager, std::string name, BDD states, int size);
    bool contains(Cube &state) const;
    std::string getName() const;
    int getSize() const;
    const BDD & getBDD() const;
};

#endif // STATESET_H
