#ifndef SETFORMULACOMPOUND_H
#define SETFORMULACOMPOUND_H

#include "setformula.h"

class SetFormulaCompound : public SetFormula
{
public:
    SetFormulaCompound();

    virtual SetFormulaType get_formula_type() = 0;
};


class SetFormulaNegation : public SetFormulaCompound {
private:
    FormulaIndex subformula_index;
public:
    SetFormulaNegation(FormulaIndex subformula_index);

    FormulaIndex get_subformula_index();

    virtual SetFormulaType get_formula_type();

};

class SetFormulaIntersection : public SetFormulaCompound {
private:
    FormulaIndex left_index;
    FormulaIndex right_index;
public:
    SetFormulaIntersection(FormulaIndex left, FormulaIndex right);

    FormulaIndex get_left_index();
    FormulaIndex get_right_index();

    virtual SetFormulaType get_formula_type();
};

class SetFormulaUnion : public SetFormulaCompound {
private:
    FormulaIndex left_index;
    FormulaIndex right_index;
public:
    SetFormulaUnion(FormulaIndex left_index, FormulaIndex right_index);

    FormulaIndex get_left_index();
    FormulaIndex get_right_index();

    virtual SetFormulaType get_formula_type();
};

class SetFormulaProgression : public SetFormulaCompound {
private:
    FormulaIndex subformula_index;
public:
    SetFormulaProgression(FormulaIndex subformula_index);

    FormulaIndex get_subformula_index();

    virtual SetFormulaType get_formula_type();

};

class SetFormulaRegression : public SetFormulaCompound {
private:
    FormulaIndex subformula_index;
public:
    SetFormulaRegression(FormulaIndex subformula_index);

    FormulaIndex get_subformula_index();

    virtual SetFormulaType get_formula_type();

};

#endif // SETFORMULACOMPOUND_H
