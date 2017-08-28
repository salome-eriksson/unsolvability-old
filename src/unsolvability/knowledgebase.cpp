#include "knowledgebase.h"

std::string EMPTYSET = "empty";
std::string UNION = "u";
std::string INTERSECTION = "^";
std::string NEGATION = "not";
std::string GOAL = "S_G";
std::string INIT_SET = "S_I";

//TODO: this is a copy from the funciton in Certificate
BDD build_bdd_from_cube(const Cube &cube) {
    assert(cube.size() == task->get_number_of_facts());
    std::vector<int> local_cube(cube.size()*2,2);
    for(size_t i = 0; i < cube.size(); ++i) {
        local_cube[i*2] = cube[i];
    }
    return BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &local_cube[0]));
}

KnowledgeBase::KnowledgeBase()
{
    unsolvability_proven = false;
    dead_sets.insert(EMPTYSET);
}

KnowledgeBase::is_dead_set(const std::string &set) {
    return dead_sets.find(set) != dead_sets.end();
}
KnowledgeBase::is_dead_state(const Cube &state) {
    BDD state_bdd = build_bdd_from_cube(state);
    return state_bdd.Leq(dead_states);
}
KnowledgeBase::is_progression(const std::string &set1, const std::string &set2) {
    return progression.find(set1 + set2) != progression.end();
}
KnowledgeBase::is_regression(const std::string &set1, const std::string &set2) {
    return regression.find(set1 + set2) != regression.end();
}
KnowledgeBase::is_subset(const std::string &set1, const std::string &set2) {
    return subsets.find(set1 + set2) != subsets.end();
}
KnowledgeBase::contains_initial(const std::string &set) {
    return has_initial.find(set) != has_initial.end();
}
KnowledgeBase::not_contains_initial(const std::string &set) {
    return has_not_initial.find(set) != has_not_initial.end();
}

KnowledgeBase::is_contained_in(const Cube &state, const std::string &set) {
    auto states_contained_in_set = contained_in.find(set);
    if(states_contained_in_set == contained_in.end()) {
        return false;
    } else {
        BDD state_bdd = build_bdd_from_cube(state);
        return state_bdd.Leq((*states_contained_in_set)->second);
    }
}

KnowledgeBase::is_unsolvability_proven() {
    return unsolvability_proven;
}



KnowledgeBase::insert_dead_set(const std::string &set) {
    dead_sets.insert(set);
}
KnowledgeBase::insert_dead_state(const Cube &state) {
    BDD state_bdd = build_bdd_from_cube(state);
    dead_states = dead_states + state_bdd;
}
KnowledgeBase::insert_progression(const std::string &set1, const std::string &set2) {
    progression.insert(set1 + set2);
}
KnowledgeBase::insert_regression(const std::string &set1, const std::string &set2) {
    regression.insert(set1 + set2);
}
KnowledgeBase::insert_subset(const std::string &set1, const std::string &set2) {
    subsets.insert(set1 + set2);
}
KnowledgeBase::insert_contains_initial(const std::string &set) {
    has_initial.insert(set);
}
KnowledgeBase::insert_not_contains_initial(const std::string &set) {
    has_not_initial.insert(set);
}
KnowledgeBase::insert_contained_in(const Cube &state, const std::string &set) {
    BDD state_bdd = build_bdd_from_cube(state);
    auto states_contained_in_set = contained_in.find(set);
    if(states_contained_in_set == contained_in.end()) {
        contained_in.insert(set, state_bdd);
    } else {
        (*states_contained_in_set)->second = (*states_contained_in_set)->second + state_bdd;
    }
}

KnowledgeBase::set_unsolvability_proven() {
    unsolvability_proven = true;
}
