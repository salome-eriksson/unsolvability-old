#ifndef SETFORMULADUALDualHorn_H
#define SETFORMULADUALDualHorn_H

#include <forward_list>
#include <unordered_set>

#include "setformulabasic.h"
#include "task.h"

class SetFormulaDualHorn;

struct DualHornConjunctionElement {
    const SetFormulaDualHorn *formula;
    std::vector<bool> removed_implications;

    DualHornConjunctionElement(const SetFormulaDualHorn *formula);
};

typedef std::vector<std::pair<const SetFormulaDualHorn *,bool>> DualHornFormulaList;
typedef std::vector<std::pair<std::forward_list<int>,std::forward_list<int>>> VariableOccurences;

class DualHornUtil {
    friend class SetFormulaDualHorn;
private:
    Task *task;
    SetFormulaDualHorn *emptyformula;
    SetFormulaDualHorn *initformula;
    SetFormulaDualHorn *goalformula;
    SetFormulaDualHorn *trueformula;
    DualHornUtil(Task *task);

    void build_actionformulas();
    bool get_DualHorn_vector(std::vector<SetFormula *> &formulas, std::vector<SetFormulaDualHorn *> &DualHorn_formulas);

    /*
     * Perform unit propagation through the conjunction of the formulas, returning whether it is satisfiable or not.
     * Partial assignment serves both as input of a partial assignment, and as output indicating which variables
     * are forced true/false by the conjunction.
     */
    bool simplify_conjunction(std::vector<DualHornConjunctionElement> &conjuncts, Cube &partial_assignment);
    bool is_restricted_satisfiable(const SetFormulaDualHorn *formula, Cube &restriction);

    bool conjunction_implies_disjunction(std::vector<SetFormulaDualHorn *> &conjuncts,
                                         std::vector<SetFormulaDualHorn *> &disjuncts);
};


/*
 * A DualHorn Formula consist of the following parts:
 *  - forced true: represents all unit clauses with one positive literal
 *  - forced false: represents all unit clauses with one negative literal
 *  - All nonunit clauses are represented by left_vars and right_side:
 *    - left_vars[i] is the negative literals of clause [i]
 *    - right_side is the positive literal (or -1 if none exists) of clause [i]
 * IMPORTANT: the clauses in left_vars/right_side are expected to contain at least 2 literals!
 *
 * left_sizes and variable_occurences are helper infos for checking satisfiability
 *   - left_sizes stores for each clause how many negative literals it contains
 *   - variable occurences stores for each variable, in which clause it appears
 *     (the first unordered_set is for negative occurence, the second for positive)
 */
class SetFormulaDualHorn : public SetFormulaBasic
{
    friend class DualHornUtil;
private:
    std::vector<std::vector<int>> left_vars;
    std::vector<int> left_sizes;
    std::vector<int> right_side;
    VariableOccurences variable_occurences;
    std::vector<int> forced_true;
    std::vector<int> forced_false;
    int varamount;
    std::vector<int> varorder;

    static DualHornUtil *util;

    /*
     * Private constructors can only be called by SetFormulaDualHorn or DualHornUtil,
     * meaning the static member SetFormulaDualHorn::util is already initialized
     * and we thus do not need to worry about initializing it.
     */
    // this constructor is used for setting up the formulas in util
    SetFormulaDualHorn(const std::vector<std::pair<std::vector<int>,int>> &clauses, int varamount);
    // used for getting a simplified conjunction of several (possibly primed) formulas
    SetFormulaDualHorn(std::vector<SetFormulaDualHorn *> &formulas);
    SetFormulaDualHorn(const SetFormulaDualHorn &other, const Action &action, bool progression);
    void simplify();
public:
    // TODO: this is currently only used for a dummy initialization
    SetFormulaDualHorn(Task *task);
    SetFormulaDualHorn(std::ifstream &input, Task *task);
    virtual ~SetFormulaDualHorn() {}

    //void shift(std::vector<int> &vars);

    int get_size() const;
    int get_varamount() const;
    int get_left(int index) const;
    const std::vector<int> &get_left_sizes() const;
    const std::vector<int> &get_left_vars(int index) const;
    int get_right(int index) const;
    const std::vector<int> &get_right_sides() const;
    const std::forward_list<int> &get_variable_occurence_left(int var) const;
    const std::forward_list<int> &get_variable_occurence_right(int var) const;
    const std::vector<int> & get_forced_true() const;
    const std::vector<int> & get_forced_false() const;
    const bool is_satisfiable() const;
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
    virtual const std::vector<int> &get_varorder();

    virtual bool supports_implicant_check() { return true; }
    virtual bool supports_clausal_entailment_check() { return true; }
    virtual bool supports_dnf_enumeration() { return false; }
    virtual bool supports_cnf_enumeration() { return true; }
    virtual bool supports_model_enumeration() { return true; }
    virtual bool supports_model_counting() { return false; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const ;
    virtual bool is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant);
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause);
    virtual bool get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause);
    virtual int get_model_count();
};

#endif // SETFORMULADUALDualHorn_H
