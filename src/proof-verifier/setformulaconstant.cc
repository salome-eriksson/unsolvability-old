#include "setformulaconstant.h"

#include <fstream>
#include <iostream>

#include "setformulahorn.h"
#include "global_funcs.h"

SetFormulaConstant::SetFormulaConstant(std::stringstream &input, Task &task)
    : task(task) {
    std::string constant_type;
    input >> constant_type;
    if(constant_type.compare("e") == 0) {
        constanttype = ConstantType::EMPTY;
    } else if(constant_type.compare("i") == 0) {
        constanttype = ConstantType::INIT;
    } else if(constant_type.compare("g") == 0) {
        constanttype = ConstantType::GOAL;
    } else {
        std::cerr << "unknown constant type " << constant_type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
}

StateSetVariable *SetFormulaConstant::find_first_non_constant_variable(std::vector<StateSetVariable *> &vec) {
    for (StateSetVariable *var : vec) {
        if (!var->is_constant()) {
            return var;
        }
    }
    return nullptr;
}

// TODO: we currently do not handle statements with only constant variables

bool SetFormulaConstant::check_statement_b1(std::vector<StateSetVariable *> &left,
                                            std::vector<StateSetVariable *> &right) {
    StateSetVariable *non_constant;
    for (auto vec : { left, right }) {
        non_constant = find_first_non_constant_variable(vec);
        if (non_constant)  {
            break;
        }
    }
    if (!non_constant) {
        std::cerr << "Statements should not involve only constant StateSets" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    return non_constant->check_statement_b1(left, right);
}

bool SetFormulaConstant::check_statement_b2(std::vector<StateSetVariable *> &prog,
                                            std::vector<StateSetVariable *> &left,
                                            std::vector<StateSetVariable *> &right,
                                            std::unordered_set<int> &actions) {
    StateSetVariable *non_constant;
    for (auto vec : { prog, left, right }) {
        non_constant = find_first_non_constant_variable(vec);
        if (non_constant)  {
            break;
        }
    }
    if (!non_constant) {
        std::cerr << "Statements should not involve only constant StateSets" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    return non_constant->check_statement_b2(prog, left, right, actions);
}

bool SetFormulaConstant::check_statement_b3(std::vector<StateSetVariable *> &reg,
                                            std::vector<StateSetVariable *> &left,
                                            std::vector<StateSetVariable *> &right,
                                            std::unordered_set<int> &actions) {
    StateSetVariable *non_constant;
    for (auto vec : { reg, left, right }) {
        non_constant = find_first_non_constant_variable(vec);
        if (non_constant)  {
            break;
        }
    }
    if (!non_constant) {
        std::cerr << "Statements should not involve only constant StateSets" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    return non_constant->check_statement_b3(reg, left, right, actions);
}

bool SetFormulaConstant::check_statement_b4(StateSetVariable *, bool, bool) {
    std::cerr << "Statement B4 should not involve constants (use B1 instead)" << std::endl;
    exit_with(ExitCode::CRITICAL_ERROR);
}

ConstantType SetFormulaConstant::get_constant_type() {
    return constanttype;
}

const std::vector<int> &SetFormulaConstant::get_varorder() {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::is_contained(const std::vector<bool> &model) const {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

int SetFormulaConstant::get_model_count() {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

StateSetBuilder<SetFormulaConstant> constant_builder("c");
