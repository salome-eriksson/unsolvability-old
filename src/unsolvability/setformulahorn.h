#ifndef SETFORMULAHORN_H
#define SETFORMULAHORN_H

#include <unordered_set>

#include "setformulabasic.h"
#include "task.h"

class SetFormulaHorn;

typedef std::vector<std::pair<const SetFormulaHorn *,bool>> HornFormulaList;
typedef std::vector<std::pair<std::unordered_set<int>,std::unordered_set<int>>> VariableOccurence;

class HornUtil {
    friend class SetFormulaHorn;
private:
    SetFormulaHorn *emptyformula;
    SetFormulaHorn *initformula;
    SetFormulaHorn *goalformula;
    SetFormulaHorn *trueformula;
    std::vector<SetFormulaHorn *> actionformulas;
    HornUtil(Task *task);
};


class SetFormulaHorn : public SetFormulaBasic
{
private:
    std::vector<std::vector<int>> left_vars;
    std::vector<int> left_sizes;
    std::vector<int> right_side;
    VariableOccurence variable_occurences;
    std::vector<int> forced_true;
    std::vector<int> forced_false;
    int varamount;

    static HornUtil *util;

    // this constructor is used for setting up the formulas in util
    SetFormulaHorn(const std::vector<std::pair<std::vector<int>,int>> &clauses, int varamount);
    // this method should be called from every constructor!
    void simplify();
public:
    SetFormulaHorn(std::ifstream input, Task *task);

    int get_size() const;
    int get_varamount() const;
    int get_left(int index) const;
    const std::vector<int> &get_left_sizes() const;
    const std::vector<int> &get_left_vars(int index) const;
    int get_right(int index) const;
    const std::unordered_set<int> &get_variable_occurence_left(int var) const;
    const std::unordered_set<int> &get_variable_occurence_right(int var) const;
    const std::vector<int> & get_forced_true() const;
    const std::vector<int> & get_forced_false() const;
    void dump() const;

    static bool is_satisfiable(const HornFormulaList &formulas, bool partial = false);
    static bool is_satisfiable(const HornFormulaList &formulas, Cube &solution, bool partial = false);
    static bool is_restricted_satisfiable(const HornFormulaList &formulas, const Cube &restrictions, bool partial = false);
    // restriction is a partial assignment to the variables (0 = false, 1 = true, 2 = dont care).
    // Any content in solution will get overwritten (even if the formula is not satisfiable).
    // If partial is true, then the solution will return 0 for forced false vars, 1 for forced true, and
    // 2 for others. A concrete solution would be to set all don't cares to false
    static bool is_restricted_satisfiable(const HornFormulaList &formulas, const Cube &restrictions,
                                   Cube &solution, bool partial= false);
    static bool implies(const HornFormulaList &formulas,
                        const SetHornFormula *right, bool right_primed);
    static bool implies_union(const HornFormulaList &formulas,
                                      const SetFormulaHorn *union_left, bool left_primed,
                                      const SetFormulaHorn *union_right, bool right_primed);
    virtual bool is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool is_subset(SetFormula *f1, SetFormula *f2);
    virtual bool intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool progression_is_union_subset(SetFormula *f, bool f_negated);
    virtual bool regression_is_union_subset(SetFormula *f, bool f_negated);

    SetFormulaType get_formula_type();
    virtual SetFormulaBasic *get_constant_formula(SetFormulaConstant *c_formula);
};

#endif // SETFORMULAHORN_H
