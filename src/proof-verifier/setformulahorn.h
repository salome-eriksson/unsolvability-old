#ifndef SETFORMULAHORN_H
#define SETFORMULAHORN_H

#include <unordered_set>

#include "setformulabasic.h"
#include "task.h"

class SetFormulaHorn;

struct HornConjunctionElement {
    const SetFormulaHorn *formula;
    const bool primed;
    std::vector<int> removed_implications;

    HornConjunctionElement (SetFormulaHorn *formula, bool primed)
        : formula(formula), primed(primed) {
    }
};

typedef std::vector<std::pair<const SetFormulaHorn *,bool>> HornFormulaList;
typedef std::vector<std::pair<std::unordered_set<int>,std::unordered_set<int>>> VariableOccurence;

class HornUtil {
    friend class SetFormulaHorn;
private:
    Task *task;
    SetFormulaHorn *emptyformula;
    SetFormulaHorn *initformula;
    SetFormulaHorn *goalformula;
    SetFormulaHorn *trueformula;
    std::vector<SetFormulaHorn *> actionformulas;
    HornUtil(Task *task);

    void build_actionformulas();
    bool get_horn_vector(std::vector<SetFormula *> &formulas, std::vector<SetFormulaHorn *> &horn_formulas);

    /*
     * Perform unit propagation through the conjunction of the formulas, returning whether it is satisfiable or not.
     * Partial assignment serves both as input of a partial assignment, and as output indicating which variables
     * are forced true/false by the conjunction.
     */
    bool simplify_conjunction(std::vector<HornConjunctionElement> &conjuncts, Cube &partial_assignment);
    bool is_restricted_satisfiable(SetFormulaHorn *formula, Cube &restriction);

    bool conjunction_implies_disjunction(std::vector<HornConjunctionElement> &conjuncts, std::vector<SetFormulaHorn *> &disjuncts);

    /*bool is_satisfiable(const HornFormulaList &formulas, bool partial = false);
    bool is_satisfiable(const HornFormulaList &formulas, Cube &solution, bool partial = false);
    bool is_restricted_satisfiable(const HornFormulaList &formulas, const Cube &restrictions, bool partial = false);
    // restriction is a partial assignment to the variables (0 = false, 1 = true, 2 = dont care).
    // Any content in solution will get overwritten (even if the formula is not satisfiable).
    // If partial is true, then the solution will return 0 for forced false vars, 1 for forced true, and
    // 2 for others. A concrete solution would be to set all don't cares to false
    bool is_restricted_satisfiable(const HornFormulaList &formulas, const Cube &restrictions,
                                   Cube &solution, bool partial= false);
    bool implies(const HornFormulaList &formulas,
                        const SetFormulaHorn *right, bool right_primed);
    bool implies_union(const HornFormulaList &formulas,
                                      const SetFormulaHorn *union_left, bool left_primed,
                                      const SetFormulaHorn *union_right, bool right_primed);
    bool implies_union_helper(const HornFormulaList &formulas, Cube &restrictions,
                              const SetFormulaHorn *right_formula, int right_offset,
                              const Cube &formulas_solution_partial);
                              */
};


/*
 * A Horn Formula consist of the following parts:
 *  - forced true: represents all unit clauses with one positive literal
 *  - forced false: represents all unit clauses with one negative literal
 *  - All nonunit clauses are represented by left_vars and right_side:
 *    - left_vars[i] is the negative literals of clause [i]
 *    - right_side is the positive literal (or -1 if none exists) of clause [i]
 *
 * left_sizes and variable_occurences are helper infos for checking satisfiability
 *   - left_sizes stores for each clause how many negative literals it contains
 *   - variable occurences stores for each variable, in which clause it appears
 *     (the first unordered_set is for negative occurence, the second for positive)
 */
class SetFormulaHorn : public SetFormulaBasic
{
    friend class HornUtil;
private:
    std::vector<std::vector<int>> left_vars;
    std::vector<int> left_sizes;
    std::vector<int> right_side;
    VariableOccurence variable_occurences;
    std::vector<int> forced_true;
    std::vector<int> forced_false;
    int varamount;

    static HornUtil *util;

    /*
     * Private constructors can only be called by SetFormulaHorn or HornUtil,
     * meaning the static member SetFormulaHorn::util is already initialized
     * and we thus do not need to worry about initializing it.
     */
    // this constructor is used for setting up the formulas in util
    SetFormulaHorn(const std::vector<std::pair<std::vector<int>,int>> &clauses, int varamount);
    // used for getting a simplified conjunction of several (possibly primed) formulas
    SetFormulaHorn(std::vector<HornConjunctionElement> &elements);
    void simplify();
public:
    // TODO: this is currently only used for a dummy initialization
    SetFormulaHorn(Task *task);
    SetFormulaHorn(std::ifstream &input, Task *task);
    virtual ~SetFormulaHorn() {}

    int get_size() const;
    int get_varamount() const;
    int get_left(int index) const;
    const std::vector<int> &get_left_sizes() const;
    const std::vector<int> &get_left_vars(int index) const;
    int get_right(int index) const;
    const std::vector<int> &get_right_sides() const;
    const std::unordered_set<int> &get_variable_occurence_left(int var) const;
    const std::unordered_set<int> &get_variable_occurence_right(int var) const;
    const std::vector<int> & get_forced_true() const;
    const std::vector<int> & get_forced_false() const;
    void dump() const;

    virtual bool is_subset(std::vector<SetFormula *> &left,
                           std::vector<SetFormula *> &right);
    virtual bool is_subset_with_progression(std::vector<SetFormula *> &left,
                                            std::vector<SetFormula *> &right,
                                            std::vector<SetFormula *> &prog,
                                            std::unordered_set<int> &actions);
    virtual bool is_subset_with_regression(std::vector<SetFormula *> &left,
                                           std::vector<SetFormula *> &right,
                                           std::vector<SetFormula *> &reg,
                                           std::unordered_set<int> &actions);

    virtual bool is_subset_of(SetFormula *superset, bool left_positive, bool right_positive);

    virtual SetFormulaType get_formula_type();
    virtual SetFormulaBasic *get_constant_formula(SetFormulaConstant *c_formula);
};

#endif // SETFORMULAHORN_H
