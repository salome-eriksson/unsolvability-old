#include "stateset.h"


std::map<std::string, StateSetConstructor> *StateSet::get_stateset_constructors() {
    static std::map<std::string, StateSetConstructor> stateset_constructors;
    return &stateset_constructors;
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

SetFormulaType StateSetUnion::get_formula_type() {
    return SetFormulaType::UNION;
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

SetFormulaType StateSetIntersection::get_formula_type() {
    return SetFormulaType::INTERSECTION;
}
StateSetBuilder<StateSetIntersection> intersection_builder("i");


StateSetNegation::StateSetNegation(std::stringstream &input, Task &) {
    input >> id_child;
}

int StateSetNegation::get_child_id() {
    return id_child;
}

SetFormulaType StateSetNegation::get_formula_type() {
    return SetFormulaType::NEGATION;
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

SetFormulaType StateSetProgression::get_formula_type() {
    return SetFormulaType::PROGRESSION;
}
StateSetBuilder<StateSetProgression> progression_builder("p");


StateSetRegression::StateSetRegression(std::stringstream &input, Task &) {
    input >> id_stateset;
    input >> id_actionset;
}

int StateSetRegression::get_actionset_id() {
    return id_actionset;
}

SetFormulaType StateSetRegression::get_formula_type() {
    return SetFormulaType::REGRESSION;
}

int StateSetRegression::get_stateset_id() {
    return id_stateset;
}
StateSetBuilder<StateSetRegression> regression_builder("r");
