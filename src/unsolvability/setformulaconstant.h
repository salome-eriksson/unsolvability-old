#ifndef SETFORMULACONSTANT_H
#define SETFORMULACONSTANT_H

#include "setformulabasic.h"

class SetFormulaConstant : public SetFormula
{
private:
    ConstantType constanttype;
    SetFormulaBasic *get_concrete_formula_instance(SetFormula *f1, SetFormula *f2);
public:
    SetFormulaConstant();

    virtual bool is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool is_subset(SetFormula *f1, SetFormula *f2);
    virtual bool intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool progression_is_union_subset(SetFormula *f, bool f_negated);
    virtual bool regression_is_union_subset(SetFormula *f, bool f_negated);

    virtual SetFormulaType get_formula_type();

    ConstantType get_constant_type();
};

#endif // SETFORMULACONSTANT_H
