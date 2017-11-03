#include "knowledgebase.h"
#include "global_funcs.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

std::string KnowledgeBase::UNION = "u";
std::string KnowledgeBase::INTERSECTION = "^";
std::string KnowledgeBase::NEGATION = "not";

KnowledgeBase::KnowledgeBase(Task *task, std::string filename) : task(task), manager(Cudd(task->get_number_of_facts(),0)) {
    unsolvability_proven = false;
    dead_sets.insert(special_set_string(SpecialSet::EMPTYSET));
    dead_sets.insert(special_set_string(SpecialSet::TRUESET) + " not");
    dead_states = manager.bddZero();
    std::ifstream in;
    in.open(filename);
    std::string line;
    while(std::getline(in,line)) {
        std::string setname = line;
        BDD bdd = manager.bddZero();
        int size = 0;
        std::getline(in,line);
        while(line.compare("set end") != 0) {
            Cube cube = parseCube(line, task->get_number_of_facts());
            bdd = bdd + BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), cube.data()));
            size++;
            std::getline(in,line);
        }
        state_sets.insert(std::make_pair(setname,StateSet(manager,setname,bdd,size)));
    }
}

bool KnowledgeBase::is_dead_set(const std::string &set) {
    return dead_sets.find(set) != dead_sets.end();
}
bool KnowledgeBase::is_dead_state(Cube &state) {
    assert(state.size() == task->get_number_of_facts());
    BDD state_bdd = BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), state.data()));;
    return state_bdd.Leq(dead_states);
}
bool KnowledgeBase::all_states_in_set_dead(const std::string &stateset) {
    assert(state_sets.find(stateset) != state_sets.end());
    const BDD &stateset_bdd = state_sets.find(stateset)->second.getBDD();
    return stateset_bdd.Leq(dead_states);
}

bool KnowledgeBase::is_progression(const std::string &set1, const std::string &set2) {
    return progression.find(set1 + set2) != progression.end();
}
bool KnowledgeBase::is_regression(const std::string &set1, const std::string &set2) {
    return regression.find(set1 + set2) != regression.end();
}
bool KnowledgeBase::is_subset(const std::string &set1, const std::string &set2) {
    return subsets.find(set1 + set2) != subsets.end();
}
bool KnowledgeBase::contains_initial(const std::string &set) {
    return has_initial.find(set) != has_initial.end();
}
bool KnowledgeBase::not_contains_initial(const std::string &set) {
    return has_not_initial.find(set) != has_not_initial.end();
}

bool KnowledgeBase::is_contained_in(Cube &state, const std::string &set) {
    assert(state.size() == task->get_number_of_facts());
    auto states_contained_in_set = contained_in.find(set);
    if(states_contained_in_set == contained_in.end()) {
        return false;
    } else {
        BDD state_bdd = BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), state.data()));;
        return state_bdd.Leq(states_contained_in_set->second);
    }
}

bool KnowledgeBase::is_unsolvability_proven() {
    return unsolvability_proven;
}



void KnowledgeBase::insert_dead_set(const std::string &set) {
    dead_sets.insert(set);
}
void KnowledgeBase::insert_dead_state(Cube &state) {
    assert(state.size() == task->get_number_of_facts());
    BDD state_bdd = BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), state.data()));;
    dead_states = dead_states + state_bdd;
}
void KnowledgeBase::insert_progression(const std::string &set1, const std::string &set2) {
    progression.insert(set1 + set2);
}
void KnowledgeBase::insert_regression(const std::string &set1, const std::string &set2) {
    regression.insert(set1 + set2);
}
void KnowledgeBase::insert_subset(const std::string &set1, const std::string &set2) {
    subsets.insert(set1 + set2);
}
void KnowledgeBase::insert_contains_initial(const std::string &set) {
    has_initial.insert(set);
}
void KnowledgeBase::insert_not_contains_initial(const std::string &set) {
    has_not_initial.insert(set);
}
void KnowledgeBase::insert_contained_in(Cube &state, const std::string &set) {
    assert(state.size() == task->get_number_of_facts());
    BDD state_bdd = BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), state.data()));;
    auto states_contained_in_set = contained_in.find(set);
    if(states_contained_in_set == contained_in.end()) {
        contained_in.insert(std::make_pair(set, state_bdd));
    } else {
        states_contained_in_set->second = states_contained_in_set->second + state_bdd;
    }
}

void KnowledgeBase::set_unsolvability_proven() {
    unsolvability_proven = true;
}

const StateSet & KnowledgeBase::get_state_set(std::string name) {
    assert(state_sets.find(name) != state_sets.end());
    return state_sets.find(name)->second;
}
