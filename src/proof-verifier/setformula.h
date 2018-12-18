#ifndef SETFORMULA_H
#define SETFORMULA_H

#include "task.h"

#include <unordered_set>

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


enum class ConstantType {
    EMPTY,
    GOAL,
    INIT
};

typedef int FormulaIndex;
typedef int ActionSetIndex;

class SetFormula
{
public:
    SetFormula();
    virtual ~SetFormula() {}

    /*
     * In the following thee methods, we assume that all Set Formulas
     * are of the same type (the one for which the method is called).
     * This is checked beforehand when creating the vectors.
     *
     * The methods are not static since they are virtual, we call it from
     * one of the Set Formulas in the vectors.
     */
    virtual bool is_subset(std::vector<SetFormula *> &left,
                           std::vector<SetFormula *> &right) = 0;
    virtual bool is_subset_with_progression(std::vector<SetFormula *> &left,
                                            std::vector<SetFormula *> &right,
                                            std::vector<SetFormula *> &prog,
                                            std::unordered_set<int> &actions) = 0;
    virtual bool is_subset_with_regression(std::vector<SetFormula *> &left,
                                           std::vector<SetFormula *> &right,
                                           std::vector<SetFormula *> &reg,
                                           std::unordered_set<int> &actions) = 0;

    // Here the other Formula has a different type
    virtual bool is_subset_of(SetFormula *superset, bool left_positive, bool right_positive) = 0;

    virtual SetFormulaType get_formula_type() = 0;
};

#endif // SETFORMULA_H
