#include "setformulacompound.h"

#include <iostream>

SetFormulaCompound::SetFormulaCompound() {

}

bool SetFormulaCompound::is_subset(SetFormula *, bool, bool) {
    std::cerr << "L \\subseteq L' is not supported for compound formulas L" << std::endl;
    return false;
}

bool SetFormulaCompound::is_subset(SetFormula *, SetFormula *) {
    std::cerr << "X \\subseteq X' \\cup X'' is not supported for compound formulas X" << std::endl;
    return false;
}

bool SetFormulaCompound::intersection_with_goal_is_subset(SetFormula *, bool, bool) {
    std::cerr << "L \\cap S_G(\\Pi) \\subseteq L' is not supported for compound formulas L" << std::endl;
    return false;
}

bool SetFormulaCompound::progression_is_union_subset(SetFormula *, bool) {
    std::cerr << "X[A] \\subseteq X \\cup X' is not supported for compound formulas x" << std::endl;
    return false;
}

bool SetFormulaCompound::regression_is_union_subset(SetFormula *, bool) {
    std::cerr << "[A]X \\subseteq X \\cup X' is not supported for compound formulas X" << std::endl;
    return false;
}


SetFormulaNegation::SetFormulaNegation(FormulaIndex subformula_index)
    : subformula_index(subformula_index) {

}

FormulaIndex SetFormulaNegation::get_subformula_index() {
    return subformula_index;
}

SetFormulaType SetFormulaNegation::get_formula_type() {
    return SetFormulaType::NEGATION;
}


SetFormulaIntersection::SetFormulaIntersection(FormulaIndex left_index, FormulaIndex right_index)
    : left_index(left_index), right_index(right_index) {

}

FormulaIndex SetFormulaIntersection::get_left_index() {
    return left_index;
}

FormulaIndex SetFormulaIntersection::get_right_index() {
    return right_index;
}

SetFormulaType SetFormulaIntersection::get_formula_type() {
    return SetFormulaType::INTERSECTION;
}


SetFormulaUnion::SetFormulaUnion(FormulaIndex left_index, FormulaIndex right_index)
    : left_index(left_index), right_index(right_index) {

}

FormulaIndex SetFormulaUnion::get_left_index() {
    return left_index;
}

FormulaIndex SetFormulaUnion::get_right_index() {
    return right_index;
}

SetFormulaType SetFormulaUnion::get_formula_type() {
    return SetFormulaType::UNION;
}


SetFormulaProgression::SetFormulaProgression(FormulaIndex subformula_index)
    : subformula_index(subformula_index) {

}

FormulaIndex SetFormulaProgression::get_subformula_index() {
    return subformula_index;
}

SetFormulaType SetFormulaProgression::get_formula_type() {
    return SetFormulaType::PROGRESSION;
}


SetFormulaRegression::SetFormulaRegression(FormulaIndex subformula_index)
    : subformula_index(subformula_index) {
}

FormulaIndex SetFormulaRegression::get_subformula_index() {
    return subformula_index;
}

SetFormulaType SetFormulaRegression::get_formula_type() {
    return SetFormulaType::REGRESSION;
}
