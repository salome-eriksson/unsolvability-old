#ifndef SETFORMULACONSTANT_H
#define SETFORMULACONSTANT_H

#include "stateset.h"

class SetFormulaConstant : public StateSetVariable
{
private:
    ConstantType constanttype;
    Task *task;
public:
    SetFormulaConstant(std::ifstream &input, Task *task);

    virtual bool check_statement_b1(std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right);
    virtual bool check_statement_b2(std::vector<StateSetVariable *> &progress,
                                    std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices);
    virtual bool check_statement_b3(std::vector<StateSetVariable *> &regress,
                                    std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices);
    virtual bool check_statement_b4(StateSetVariable *right,
                                    bool left_positive, bool right_positive);

    virtual SetFormulaType get_formula_type();
    virtual const std::vector<int> &get_varorder();

    ConstantType get_constant_type();

    // TODO: what should we set here?
    virtual bool supports_mo() { return true; }
    virtual bool supports_ce() { return true; }
    virtual bool supports_im() { return true; }
    virtual bool supports_me() { return true; }
    virtual bool supports_todnf() { return true; }
    virtual bool supports_tocnf() { return true; }
    virtual bool supports_ct() { return true; }
    virtual bool is_nonsuccint() { return true; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const ;
    virtual bool is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant);
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause);
    virtual bool get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause);
    virtual int get_model_count();
};

#endif // SETFORMULACONSTANT_H
