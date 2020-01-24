#include "stateset.h"

StateSetUnion::StateSetUnion(int id_left, int id_right)
    : id_left(id_left), id_right(id_right) {}

int StateSetUnion::get_left_id() {
    return id_left;
}

int StateSetUnion::get_right_id() {
    return id_right;
}

SetFormulaType StateSetUnion::get_formula_type() {
    return SetFormulaType::UNION;
}


StateSetIntersection::StateSetIntersection(int id_left, int id_right)
    : id_left(id_left), id_right(id_right) {}

int StateSetIntersection::get_left_id() {
    return id_left;
}

int StateSetIntersection::get_right_id() {
    return id_right;
}

SetFormulaType StateSetIntersection::get_formula_type() {
    return SetFormulaType::INTERSECTION;
}


StateSetNegation::StateSetNegation(int id_child)
    : id_child(id_child) {}

int StateSetNegation::get_child_id() {
    return id_child;
}

SetFormulaType StateSetNegation::get_formula_type() {
    return SetFormulaType::NEGATION;
}


StateSetProgression::StateSetProgression(int id_stateset, int id_actionset)
    : id_stateset(id_stateset), id_actionset(id_actionset) {}

int StateSetProgression::get_actionset_id() {
    return id_actionset;
}

int StateSetProgression::get_stateset_id() {
    return id_stateset;
}

SetFormulaType StateSetProgression::get_formula_type() {
    return SetFormulaType::PROGRESSION;
}


StateSetRegression::StateSetRegression(int id_stateset, int id_actionset)
    : id_stateset(id_stateset), id_actionset(id_actionset) {}

int StateSetRegression::get_actionset_id() {
    return id_actionset;
}

SetFormulaType StateSetRegression::get_formula_type() {
    return SetFormulaType::REGRESSION;
}

int StateSetRegression::get_stateset_id() {
    return id_stateset;
}
