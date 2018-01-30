#ifndef SETFORMULA_H
#define SETFORMULA_H

#include "task.h"

enum class SetFormulaType {
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
// TODO: can this be done nicer?
extern std::vector<std::string> setformulatype_strings;

enum class ConstantType {
    EMPTY,
    GOAL,
    INIT
};

typedef int FormulaIndex;

class SetFormula
{
public:
    SetFormula();
    virtual ~SetFormula() {}

    virtual bool is_subset(SetFormula *f, bool negated, bool f_negated) = 0;
    virtual bool is_subset(SetFormula *f1, SetFormula *f2) = 0;
    virtual bool intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated) = 0;
    virtual bool progression_is_union_subset(SetFormula *f, bool f_negated) = 0;
    virtual bool regression_is_union_subset(SetFormula *f, bool f_negated) = 0;

    virtual SetFormulaType get_formula_type() = 0;
};

#endif // SETFORMULA_H
