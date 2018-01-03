#ifndef SETFORMULABASIC_H
#define SETFORMULABASIC_H

#include "setformula.h"


class SetFormulaBasic : public SetFormula
{
public:
    SetFormulaBasic();

    virtual SetFormulaType get_formula_type() = 0;
    //TODO: for BDDs the varorder can be retrieved from *this
    virtual SetFormulaBasic *getConstantFormula(ConstantType c) = 0;
};

#endif // SETFORMULABASIC_H
