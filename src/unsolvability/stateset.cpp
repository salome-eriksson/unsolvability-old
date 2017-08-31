#include "stateset.h"

StateSet::StateSet(std::string name, BDD states) : name(name), states(states){

}

bool StateSet::contains(const Cube &state) {
    BDD statebdd = BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &state[0]));
    return statebdd.Leq(states);
}

std::string StateSet::getName() {
    return name;
}
