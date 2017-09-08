#include "stateset.h"

StateSet::StateSet(Cudd manager, std::string name, BDD states, int size)
    : manager(manager), name(name), states(states), size(size){

}

bool StateSet::contains(Cube &state) const{
    BDD statebdd = BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &state[0]));
    return statebdd.Leq(states);
}

std::string StateSet::getName() const{
    return name;
}

int StateSet::getSize() const{
    return size;
}

const BDD & StateSet::getBDD() const{
    return states;
}
