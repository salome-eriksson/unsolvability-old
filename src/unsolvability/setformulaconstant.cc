#include "setformulaconstant.h"
#include "setformulahorn.h"

SetFormulaConstant::SetFormulaConstant(std::string name) : name(name)
{

}

bool SetFormulaConstant::is_subset(SetFormula *f) {
    switch (f->get_formula_type()) {
    case SetFormulaType::CONSTANT:
        /* TODO: which formalism when both sides are constant? Probably Horn
         * or explicit but then we need to create a dummy formula here...
         */
        break;
    case SetFormulaType::BDD:
    case SetFormulaType::HORN:
    case SetFormulaType::TWOCNF:
    case SetFormulaType::EXPLICIT:
        SetFormulaBasic * constantformula =
                static_cast<SetFormulaBasic *>(f)->get_constant_formula(constanttype);
        return constantformula->is_subset(f);
        break;
    case SetFormulaType::NEGATION:

        break;

    default:
        std::cerr << "CRITICAL: isSubset not supported for type " << f->get_formula_type() << std::endl;
        return false;
        break;
    }
}

bool SetFormulaConstant::is_subset(SetFormula *f1, SetFormula *f2) {

}

bool SetFormulaConstant::intersection_with_goal_is_subset(SetFormula *f) {

}

bool SetFormulaConstant::progression_is_union_subset(SetFormula *f) {

}

bool SetFormulaConstant::regression_is_union_subset(SetFormula *f) {

}

SetFormulaType SetFormulaConstant::get_formula_type() {
    return SetFormulaType::CONSTANT;
}

ConstantType SetFormulaConstant::get_constant_type() {
    return constanttype;
}
