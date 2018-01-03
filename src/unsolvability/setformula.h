#ifndef SETFORMULA_H
#define SETFORMULA_H

enum SetFormulaType {
    CONSTANT,
    BDD,
    HORN,
    TWOCNF,
    EXPLICIT,
    NEGATION,
    INTERSECTION,
    UNION,
    PROGRESSION,
    REGRESSION
};

enum ConstantType {
    EMPTY,
    GOAL,
    INIT
};

typedef int FormulaIndex;

class SetFormula
{
public:
    SetFormula();

    virtual bool is_subset(SetFormula *f) = 0;
    virtual bool is_subset(SetFormula *f1, SetFormula *f2) = 0;
    virtual bool intersection_with_goal_is_subset(SetFormula *f) = 0;
    virtual bool progression_is_union_subset(SetFormula *f) = 0;
    virtual bool regression_is_union_subset(SetFormula *f) = 0;

    SetFormulaType get_formula_type() = 0;
};

#endif // SETFORMULA_H
