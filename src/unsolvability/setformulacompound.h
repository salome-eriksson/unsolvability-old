#ifndef SETFORMULACOMPOUND_H
#define SETFORMULACOMPOUND_H

#include "setformula.h"

class SetFormulaCompound : public SetFormula
{
public:
    SetFormulaCompound();

    virtual bool is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool is_subset(SetFormula *f1, SetFormula *f2);
    virtual bool intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool progression_is_union_subset(SetFormula *f, bool f_negated);
    virtual bool regression_is_union_subset(SetFormula *f, bool f_negated);

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
    SetFormulaIntersection(FormulaIndex left_index, FormulaIndex right_index);

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
