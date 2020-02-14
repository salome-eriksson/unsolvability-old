#ifndef SETFORMULAHORN_H
#define SETFORMULAHORN_H

#include <forward_list>
#include <unordered_set>

#include "stateset.h"
#include "setformulaconstant.h"
#include "task.h"

class SetFormulaHorn;

struct HornConjunctionElement {
    const SetFormulaHorn *formula;
    std::vector<bool> removed_implications;

    HornConjunctionElement(const SetFormulaHorn *formula);
};

typedef std::vector<std::pair<const SetFormulaHorn *,bool>> HornFormulaList;
typedef std::vector<std::pair<std::forward_list<int>,std::forward_list<int>>> VariableOccurences;

class HornUtil {
    friend class SetFormulaHorn;
private:
    Task &task;
    SetFormulaHorn *emptyformula;
    SetFormulaHorn *initformula;
    SetFormulaHorn *goalformula;
    SetFormulaHorn *trueformula;
    HornUtil(Task &task);

    void build_actionformulas();
    bool get_horn_vector(std::vector<StateSetVariable *> &formulas, std::vector<SetFormulaHorn *> &horn_formulas);

    /*
     * Perform unit propagation through the conjunction of the formulas, returning whether it is satisfiable or not.
     * Partial assignment serves both as input of a partial assignment, and as output indicating which variables
     * are forced true/false by the conjunction.
     */
    bool simplify_conjunction(std::vector<HornConjunctionElement> &conjuncts, Cube &partial_assignment);
    bool is_restricted_satisfiable(const SetFormulaHorn *formula, Cube &restriction);

    bool conjunction_implies_disjunction(std::vector<SetFormulaHorn *> &conjuncts,
                                         std::vector<SetFormulaHorn *> &disjuncts);
};


/*
 * A Horn Formula consist of the following parts:
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
class SetFormulaHorn : public StateSetVariable
{
    friend class HornUtil;
private:
    std::vector<std::vector<int>> left_vars;
    std::vector<int> left_sizes;
    std::vector<int> right_side;
    VariableOccurences variable_occurences;
    std::vector<int> forced_true;
    std::vector<int> forced_false;
    int varamount;
    std::vector<int> varorder;

    static HornUtil *util;

    /*
     * Private constructors can only be called by SetFormulaHorn or HornUtil,
     * meaning the static member SetFormulaHorn::util is already initialized
     * and we thus do not need to worry about initializing it.
     */
    // this constructor is used for setting up the formulas in util
    SetFormulaHorn(const std::vector<std::pair<std::vector<int>,int>> &clauses, int varamount);
    // used for getting a simplified conjunction of several (possibly primed) formulas
    SetFormulaHorn(std::vector<SetFormulaHorn *> &formulas);
    SetFormulaHorn(const SetFormulaHorn &other, const Action &action, bool progression);
    void simplify();
public:
    // TODO: this is currently only used for a dummy initialization
    SetFormulaHorn(Task &task);
    SetFormulaHorn(std::stringstream &input, Task &task);
    virtual ~SetFormulaHorn() {}

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

    virtual SetFormulaType get_formula_type();
    virtual StateSetVariable *get_constant_formula(SetFormulaConstant *c_formula);
    virtual const std::vector<int> &get_varorder();

    virtual bool check_statement_b1(std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right);
    virtual bool check_statement_b2(std::vector<StateSetVariable *> &progress,
                                    std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices);
    virtual bool check_statement_b3(std::vector<StateSetVariable *> &regress,
                                    std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices);
    virtual bool check_statement_b4(StateSetVariable *right,
                                    bool left_positive, bool right_positive);

    virtual bool supports_mo() { return true; }
    virtual bool supports_ce() { return true; }
    virtual bool supports_im() { return true; }
    virtual bool supports_me() { return true; }
    virtual bool supports_todnf() { return false; }
    virtual bool supports_tocnf() { return false; }
    virtual bool supports_ct() { return false; }
    virtual bool is_nonsuccint() { return false; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const ;
    virtual bool is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant);
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause);
    virtual bool get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause);
    virtual int get_model_count();
};

#endif // SETFORMULAHORN_H
