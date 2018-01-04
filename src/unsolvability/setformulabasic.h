#ifndef SETFORMULABASIC_H
#define SETFORMULABASIC_H

#include "setformula.h"


class SetFormulaBasic : public SetFormula
{
public:
    SetFormulaBasic();

    virtual SetFormulaBasic *get_constant_formula(ConstantType c) = 0;
};

#endif // SETFORMULABASIC_H
