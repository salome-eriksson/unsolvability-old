#include "setformulaconstant.h"
#include "setformulahorn.h"

SetFormulaConstant::SetFormulaConstant() {

}

SetFormulaBasic *SetFormulaConstant::get_concrete_formula_instance(SetFormula *f1, SetFormula *f2) {
    SetFormula *fx = f1;
    if(fx->get_formula_type() == SetFormulaType::CONSTANT && f2) {
        fx = f2;
    }
    switch (fx->get_formula_type()) {
    case SetFormulaType::CONSTANT:
        // TODO: This is somewhat of a hack with the dummy variable, but this way we ensure
        // that the static member of SetFormulaHorn is initialized properly, and that we can
        // access get_constant_formula()
        SetFormulaHorn dummy;
        return dummy.get_constant_formula(constanttype);
        break;
    case SetFormulaType::BDD:
    case SetFormulaType::HORN:
    case SetFormulaType::TWOCNF:
    case SetFormulaType::EXPLICIT:
        return static_cast<SetFormulaBasic *>(fx)->get_constant_formula(constanttype);
        break;
    default:
        return nullptr;
    }
}

bool SetFormulaConstant::is_subset(SetFormula *f, bool negated, bool f_negated) {
    SetFormulaBasic *concrete_instance = get_concrete_formula_instance(f, nullptr);
    if(!concrete_instance) {
        std::cerr << "L \subseteq L' is not supported for L' with type "
                  << f->get_formula_type() << std::endl;
        return false;
    }
    return concrete_instance->is_subset(f, negated, f_negated);
}

bool SetFormulaConstant::is_subset(SetFormula *f1, SetFormula *f2) {
    SetFormulaBasic *concrete_instance = get_concrete_formula_instance(f1, f2);
    if(!concrete_instance) {
        std::cerr << "X \subseteq X' \cup X'' is not supported for X' with type "
                  << f1->get_formula_type() << "and X'' with type " << f2->get_formula_type() << std::endl;
        return false;
    }
    return concrete_instance->is_subset(f1, f2);
}

bool SetFormulaConstant::intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated) {
    SetFormulaBasic *concrete_instance = get_concrete_formula_instance(f, nullptr);
    if(!concrete_instance) {
        std::cerr << "L \cap S_G(\Pi) \subseteq L' is not supported for X' with type"
                  << f->get_formula_type() << std::endl;
    }
    return concrete_instance->intersection_with_goal_is_subset(f, negated, f_negated);
}

bool SetFormulaConstant::progression_is_union_subset(SetFormula *f, bool f_negated) {
    SetFormulaBasic *concrete_instance = get_concrete_formula_instance(f, nullptr);
    if(!concrete_instance) {
        std::cerr << "X[A] \subseteq X \cup X' is not supported for X' with type"
                  << f->get_formula_type() << std::endl;
    }
    return concrete_instance->progression_is_union_subset(f, f_negated);
}

bool SetFormulaConstant::regression_is_union_subset(SetFormula *f, bool f_negated) {
    SetFormulaBasic *concrete_instance = get_concrete_formula_instance(f, nullptr);
    if(!concrete_instance) {
        std::cerr << "[A]X \subseteq X \cup X' is not supported for L' with type"
                  << f->get_formula_type() << std::endl;
    }
    return concrete_instance->regression_is_union_subset(f, f_negated);
}

SetFormulaType SetFormulaConstant::get_formula_type() {
    return SetFormulaType::CONSTANT;
}

ConstantType SetFormulaConstant::get_constant_type() {
    return constanttype;
}
