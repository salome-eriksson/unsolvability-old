#include "stateset.h"

StateSet::StateSet(std::string name, BDD states, int size)
    : name(name), states(states), size(size){

}

bool StateSet::contains(const Cube &state) {
    BDD statebdd = BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &state[0]));
    return statebdd.Leq(states);
}

std::string StateSet::getName() {
    return name;
}

int StateSet::getSize() {
    return size;
}
