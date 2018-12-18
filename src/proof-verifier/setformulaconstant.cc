#include "setformulaconstant.h"

#include <fstream>
#include <iostream>

#include "setformulahorn.h"
#include "global_funcs.h"

SetFormulaConstant::SetFormulaConstant(std::ifstream &input, Task *task)
    : task(task) {
    std::string type;
    input >> type;
    if(type.compare("e") == 0) {
        constanttype = ConstantType::EMPTY;
    } else if(type.compare("i") == 0) {
        constanttype = ConstantType::INIT;
    } else if(type.compare("g") == 0) {
        constanttype = ConstantType::GOAL;
    } else {
        std::cerr << "unknown constant type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
}

SetFormulaBasic *SetFormulaConstant::get_concrete_formula_instance(SetFormula *f1, SetFormula *f2) {
    SetFormula *fx = f1;
    if(fx->get_formula_type() == SetFormulaType::CONSTANT && f2) {
        fx = f2;
    }
    switch (fx->get_formula_type()) {
    case SetFormulaType::CONSTANT:{
        // TODO: This is somewhat of a hack with the dummy variable, but this way we ensure
        // that the static member of SetFormulaHorn is initialized properly, and that we can
        // access get_constant_formula()
        SetFormulaHorn dummy(task);
        return dummy.get_constant_formula(this);
        break;
    }
    case SetFormulaType::BDD:
    case SetFormulaType::HORN:
    case SetFormulaType::TWOCNF:
    case SetFormulaType::EXPLICIT:
        return static_cast<SetFormulaBasic *>(fx)->get_constant_formula(this);
        break;
    default:
        return nullptr;
    }
}

bool SetFormulaConstant::is_subset(std::vector<SetFormula *> &,
                       std::vector<SetFormula *> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::is_subset_with_progression(std::vector<SetFormula *> &,
                                                    std::vector<SetFormula *> &,
                                                    std::vector<SetFormula *> &,
                                                    std::unordered_set<int> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::is_subset_with_regression(std::vector<SetFormula *> &,
                                                   std::vector<SetFormula *> &,
                                                   std::vector<SetFormula *> &,
                                                   std::unordered_set<int> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::is_subset_of(SetFormula *, bool, bool) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

SetFormulaType SetFormulaConstant::get_formula_type() {
    return SetFormulaType::CONSTANT;
}

ConstantType SetFormulaConstant::get_constant_type() {
    return constanttype;
}
