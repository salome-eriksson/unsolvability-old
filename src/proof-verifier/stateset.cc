#include "stateset.h"

StateSet::~StateSet() {}

std::map<std::string, StateSetConstructor> *StateSet::get_stateset_constructors() {
    static std::map<std::string, StateSetConstructor> stateset_constructors;
    return &stateset_constructors;
}


bool StateSet::gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &,
                                      std::vector<StateSetVariable *> &,
                                      std::vector<StateSetVariable *> &, bool) {
    return false;
}

bool StateSet::gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &,
                                             std::vector<StateSetVariable *> &,
                                             std::vector<StateSetVariable *> &, bool) {
    return false;
}


bool StateSetVariable::gather_union_variables(const std::deque<std::unique_ptr<StateSet> > &,
                                              std::vector<StateSetVariable *> &positive,
                                              std::vector<StateSetVariable *> &, bool) {
    positive.push_back(this);
    return true;
}

bool StateSetVariable::gather_intersection_variables(const std::deque<std::unique_ptr<StateSet> > &,
                                                     std::vector<StateSetVariable *> &positive,
                                                     std::vector<StateSetVariable *> &, bool) {
    positive.push_back(this);
    return true;
}

bool StateSetVariable::is_constant() {
    return false;
}


StateSetUnion::StateSetUnion(std::stringstream &input, Task &) {
    input >> id_left;
    input >> id_right;
}

int StateSetUnion::get_left_id() {
    return id_left;
}

int StateSetUnion::get_right_id() {
    return id_right;
}

bool StateSetUnion::gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                           std::vector<StateSetVariable *> &positive,
                                           std::vector<StateSetVariable *> &negative,
                                           bool must_be_variable) {
    if (must_be_variable) {
        return false;
    }
    return formulas[id_left]->gather_union_variables(formulas, positive, negative, false) &&
           formulas[id_right]->gather_union_variables(formulas, positive, negative, false);
}
StateSetBuilder<StateSetUnion> union_builder("u");


StateSetIntersection::StateSetIntersection(std::stringstream &input, Task &) {
    input >> id_left;
    input >> id_right;
}

int StateSetIntersection::get_left_id() {
    return id_left;
}

int StateSetIntersection::get_right_id() {
    return id_right;
}

bool StateSetIntersection::gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                                         std::vector<StateSetVariable *> &positive,
                                                         std::vector<StateSetVariable *> &negative,
                                                         bool must_be_variable) {
    if (must_be_variable) {
        return false;
    }
    return formulas[id_left]->gather_intersection_variables(formulas, positive, negative, false) &&
           formulas[id_right]->gather_intersection_variables(formulas, positive, negative, false);
}
StateSetBuilder<StateSetIntersection> intersection_builder("i");


StateSetNegation::StateSetNegation(std::stringstream &input, Task &) {
    input >> id_child;
}

int StateSetNegation::get_child_id() {
    return id_child;
}

bool StateSetNegation::gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                              std::vector<StateSetVariable *> &positive,
                                              std::vector<StateSetVariable *> &negative,
                                              bool must_be_variable) {
    if (must_be_variable) {
        return false;
    }
    return formulas[id_child]->gather_union_variables(formulas, negative, positive, true);
}

bool StateSetNegation::gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                                     std::vector<StateSetVariable *> &positive,
                                                     std::vector<StateSetVariable *> &negative,
                                                     bool must_be_variable) {
    if (must_be_variable) {
        return false;
    }
    return formulas[id_child]->gather_intersection_variables(formulas, negative, positive, true);
}
StateSetBuilder<StateSetNegation> negation_builder("n");


StateSetProgression::StateSetProgression(std::stringstream &input, Task &) {
    input >> id_stateset;
    input >> id_actionset;
}

int StateSetProgression::get_actionset_id() {
    return id_actionset;
}

int StateSetProgression::get_stateset_id() {
    return id_stateset;
}
StateSetBuilder<StateSetProgression> progression_builder("p");


StateSetRegression::StateSetRegression(std::stringstream &input, Task &) {
    input >> id_stateset;
    input >> id_actionset;
}

int StateSetRegression::get_actionset_id() {
    return id_actionset;
}

int StateSetRegression::get_stateset_id() {
    return id_stateset;
}
StateSetBuilder<StateSetRegression> regression_builder("r");
