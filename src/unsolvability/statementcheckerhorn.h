#ifndef STATEMENTCHECKERHORN_H
#define STATEMENTCHECKERHORN_H

#include "statementchecker.h"
#include <unordered_set>

// TODO: These two classes need some refactoring!!

class HornFormula;
typedef std::vector<std::pair<const HornFormula *,bool>> HornFormulaList;

class HornFormula {
private:
    std::vector<int> left_size;
    std::vector<std::vector<int>> left_vars;
    std::vector<int> right_side;
    std::vector<std::unordered_set<int>> variable_occurences;
    std::vector<int> forced_true;
    std::vector<int> forced_false;
    int varamount;

    // these two methods should be called from every constructor!
    void simplify();
    void set_left_vars();
public:
    // Horn formulas implicitly simplify themselves when constructed
    HornFormula(std::string input, int varamount);
    HornFormula(std::vector<int> &left_size, std::vector<int> &right_side,
                std::vector<std::unordered_set<int>> &variable_occurences, int varamount);
    HornFormula(const HornFormulaList &subformulas);
    int get_size() const;
    int get_varamount() const;
    const std::vector<int> &get_left_sizes() const;
    int get_left(int index) const;
    const std::vector<int> &get_left_vars(int index) const;
    int get_right(int index) const;
    const std::unordered_set<int> &get_variable_occurence(int var) const;
    const std::vector<int> & get_forced_true() const;
    const std::vector<int> & get_forced_false() const;
    void dump() const;
};


class StatementCheckerHorn : public StatementChecker
{
private:
    std::vector<HornFormula> action_formulas;
    std::unordered_map<std::string,HornFormula> stored_formulas;
    void read_in_composite_formulas(std::ifstream &in);
    HornFormulaList get_formulalist(const std::string &description);
    bool is_satisfiable(const HornFormulaList &formulas, bool partial = false);
    bool is_satisfiable(const HornFormulaList &formulas, Cube &solution, bool partial = false);
    bool is_restricted_satisfiable(const HornFormulaList &formulas, const Cube &restrictions, bool partial = false);
    // restriction is a partial assignment to the variables (0 = false, 1 = true, 2 = dont care).
    // Any content in solution will get overwritten (even if the formula is not satisfiable).
    // If partial is true, then the solution will return 0 for forced false vars, 1 for forced true, and
    // 2 for others. A concrete solution would be to set all don't cares to false
    bool is_restricted_satisfiable(const HornFormulaList &formulas, const Cube &restrictions,
                                   Cube &solution, bool partial= false);
    bool implies(const HornFormulaList &formulas, const HornFormulaList &right);
public:
    StatementCheckerHorn(KnowledgeBase *kb, Task *task, std::ifstream &in);
    virtual bool check_subset(const std::string &set1, const std::string &set2);
    // supported only if set1 is a Horn formula and set2 is the negation of a Horn formula
    virtual bool check_progression(const std::string &set1, const std::string &set2);
    // supported only if set1 is a Horn formula and set2 is the negation of a Horn formula
    virtual bool check_regression(const std::string &set1, const std::string &set2);
    virtual bool check_is_contained(Cube &state, const std::string &set);
    virtual bool check_initial_contained(const std::string &set);
    virtual bool check_set_subset_to_stateset(const std::string &set, const StateSet &stateset);
};

#endif // STATEMENTCHECKERHORN_H
