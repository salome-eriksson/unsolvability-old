#include "setformulahorn.h"

SpecialFormulasHorn::SpecialFormulasHorn(Task *task) {

}

SetFormulaHorn::SetFormulaHorn()
{

    if (specialformulas == nullptr) {
        // set up special formulas
    }

}

bool SetFormulaHorn::is_subset(SetFormula *f) {

}

bool SetFormulaHorn::is_subset(SetFormula *f1, SetFormula *f2) {

}

bool SetFormulaHorn::intersection_with_goal_is_subset(SetFormula *f) {

}

bool SetFormulaHorn::progression_is_union_subset(SetFormula *f) {

}

bool SetFormulaHorn::regression_is_union_subset(SetFormula *f) {

}


bool SetFormulaHorn::get_formula_type() {
    return SetFormulaType::HORN;
}

SetFormulaBasic *SetFormulaHorn::get_constant_formula(ConstantType c) {
    return constantformulas[c];
}
