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

bool SetFormulaConstant::check_statement_b1(std::vector<StateSetVariable *> &,
                                            std::vector<StateSetVariable *> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::check_statement_b2(std::vector<StateSetVariable *> &,
                                            std::vector<StateSetVariable *> &,
                                            std::vector<StateSetVariable *> &,
                                            std::unordered_set<int> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::check_statement_b3(std::vector<StateSetVariable *> &,
                                            std::vector<StateSetVariable *> &,
                                            std::vector<StateSetVariable *> &,
                                            std::unordered_set<int> &) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaConstant::check_statement_b4(StateSetVariable *, bool, bool) {
    std::cerr << "subset checks should not be forwarded to SetFormulaConstant";
    exit_with(ExitCode::CRITICAL_ERROR);
}

SetFormulaType SetFormulaConstant::get_formula_type() {
    return SetFormulaType::CONSTANT;
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
