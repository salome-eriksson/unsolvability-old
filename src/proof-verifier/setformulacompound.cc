#include "setformulacompound.h"

#include "global_funcs.h"

#include <iostream>

SetFormulaCompound::SetFormulaCompound() {

}

bool SetFormulaCompound::is_subset(std::vector<SetFormula *> &,
                       std::vector<SetFormula *> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaCompound";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaCompound::is_subset_with_progression(std::vector<SetFormula *> &,
                                                    std::vector<SetFormula *> &,
                                                    std::vector<SetFormula *> &,
                                                    std::unordered_set<int> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaCompound";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaCompound::is_subset_with_regression(std::vector<SetFormula *> &,
                                                   std::vector<SetFormula *> &,
                                                   std::vector<SetFormula *> &,
                                                   std::unordered_set<int> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaCompound";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaCompound::is_subset_of(SetFormula *, bool, bool) {
    std::cerr << "subset checks should not be forwarded to SetFormulaCompound";
    exit_with(ExitCode::CRITICAL_ERROR);
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


SetFormulaProgression::SetFormulaProgression(
        FormulaIndex subformula_index, ActionSetIndex actionset_index)
    : subformula_index(subformula_index), actionset_index(actionset_index) {

}

FormulaIndex SetFormulaProgression::get_subformula_index() {
    return subformula_index;
}

ActionSetIndex SetFormulaProgression::get_actionset_index() {
    return actionset_index;
}

SetFormulaType SetFormulaProgression::get_formula_type() {
    return SetFormulaType::PROGRESSION;
}


SetFormulaRegression::SetFormulaRegression(
        FormulaIndex subformula_index, ActionSetIndex actionset_index)
    : subformula_index(subformula_index), actionset_index(actionset_index) {
}

FormulaIndex SetFormulaRegression::get_subformula_index() {
    return subformula_index;
}

ActionSetIndex SetFormulaRegression::get_actionset_index() {
    return actionset_index;
}

SetFormulaType SetFormulaRegression::get_formula_type() {
    return SetFormulaType::REGRESSION;
}
