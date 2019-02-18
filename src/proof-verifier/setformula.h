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
    virtual const std::vector<int> &get_varorder() = 0;

    virtual bool supports_implicant_check() = 0;
    virtual bool supports_clausal_entailment_check() = 0;
    virtual bool supports_dnf_enumeration() = 0;
    virtual bool supports_cnf_enumeration() = 0;
    virtual bool supports_model_counting() = 0;

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const = 0;
    virtual bool is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) = 0;
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) = 0;
    // returns false if no clause with index i exists
    virtual bool get_clause(int i, std::vector<int> &varorder, std::vector<bool> &clause) = 0;
    virtual int get_model_count() = 0;
};

#endif // SETFORMULA_H
