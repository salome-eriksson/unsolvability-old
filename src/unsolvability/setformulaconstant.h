#ifndef SETFORMULACONSTANT_H
#define SETFORMULACONSTANT_H

#include "setformulabasic.h"

class SetFormulaConstant : public SetFormula
{
    ConstantType constanttype;
public:
    SetFormulaConstant(std::string name);

    virtual bool is_subset(SetFormula *f);
    virtual bool is_subset(SetFormula *f1, SetFormula *f2);
    virtual bool intersection_with_goal_is_subset(SetFormula *f);
    virtual bool progression_is_union_subset(SetFormula *f);
    virtual bool regression_is_union_subset(SetFormula *f);

    virtual SetFormulaType get_formula_type();

    ConstantType get_constant_type();
};

#endif // SETFORMULACONSTANT_H
