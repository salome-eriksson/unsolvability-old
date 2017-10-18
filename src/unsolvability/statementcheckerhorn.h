#ifndef STATEMENTCHECKERHORN_H
#define STATEMENTCHECKERHORN_H

#include "statementchecker.h"

class Implication {
private:
    std::vector<int> left;
    int right;
public:
    Implication(std::vector<int> left, int right);
    const std::vector<int> &get_left() const;
    int get_right() const;
};

struct HornFormula {
private:
    int varamount;
    std::vector<Implication> implications;
    std::vector<std::vector<int>> variable_occurences;
public:
    HornFormula(std::vector<Implication> implications, int varamount);
    // combines several Horn formulas into one, with the possibility of "priming" certain
    // subformulas (that is, using a "primed" copy of each variable in these subformulas)
    // expects all subformulas to have the same varamount, primed variables will have indexes
    // [varamount, 2*varamount)
    HornFormula(std::vector<std::pair<HornFormula *,bool>> &subformulas);
    const std::vector<int> &get_variable_occurence(int var) const;
    int get_size() const;
    int get_varamount() const;
    std::vector<Implication>::const_iterator begin() const;
    std::vector<Implication>::const_iterator end() const;
    const Implication &get_implication(int index) const;
};

class StatementCheckerHorn : public StatementChecker
{
private:
    std::vector<HornFormula> action_formulas;
    std::unordered_map<std::string,HornFormula> formulas;
    bool is_satisfiable(const HornFormula &formula);
    bool is_satisfiable(const HornFormula &formula, Cube &solution);
    bool is_satisfiable(const HornFormula &formula, const Cube &restrictions);
    // restriction is a partial assignment to the variables (0 = false, 1 = true, 2 = dont care),
    // which needs to be the same length as the formula.
    // any content in solution will get overwritten (even if the formula is not satisfiable)
    bool is_satisfiable(const HornFormula &formula, const Cube &restrictions, Cube &solution);
    bool implies(const HornFormula &formula1, const HornFormula &formula2);
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
