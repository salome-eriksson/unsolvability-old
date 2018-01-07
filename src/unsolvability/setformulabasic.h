#ifndef SETFORMULABASIC_H
#define SETFORMULABASIC_H

#include "setformula.h"
#include "setformulaconstant.h"

class SetFormulaBasic : public SetFormula
{
public:
    SetFormulaBasic();

    virtual SetFormulaBasic *get_constant_formula(SetFormulaConstant *c_formula) = 0;
};

#endif // SETFORMULABASIC_H
