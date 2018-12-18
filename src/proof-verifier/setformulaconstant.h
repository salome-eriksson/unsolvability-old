#ifndef SETFORMULACONSTANT_H
#define SETFORMULACONSTANT_H

#include "setformula.h"

class SetFormulaBasic;

class SetFormulaConstant : public SetFormula
{
private:
    ConstantType constanttype;
    Task *task;
    /* returns an actual representation of the constant set:
     *  - if f1 is a basic setformula, then the type of f1 is chosen
     *  - if f1 is a constant formula and f2 is a basic setformula, then the type of f2 is chosen
     *  - if f1 and f2 are constant formulas, then Horn is chosen
     *  - else (f1 and/or f2 are compound formulas) a nullptr is returned
     * f2 can be a nullptr
     */
    SetFormulaBasic *get_concrete_formula_instance(SetFormula *f1, SetFormula *f2);
public:
    SetFormulaConstant(std::ifstream &input, Task *task);

    virtual bool is_subset(std::vector<SetFormula *> &left,
                           std::vector<SetFormula *> &right);
    virtual bool is_subset_with_progression(std::vector<SetFormula *> &left,
                                            std::vector<SetFormula *> &right,
                                            std::vector<SetFormula *> &prog,
                                            std::unordered_set<int> &);
    virtual bool is_subset_with_regression(std::vector<SetFormula *> &left,
                                           std::vector<SetFormula *> &right,
                                           std::vector<SetFormula *> &reg,
                                           std::unordered_set<int> &);

    virtual bool is_subset_of(SetFormula *superset, bool left_positive, bool right_positive);

    virtual SetFormulaType get_formula_type();

    ConstantType get_constant_type();
};

#endif // SETFORMULACONSTANT_H
