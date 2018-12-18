#ifndef SETFORMULAEXPLICIT_H
#define SETFORMULAEXPLICIT_H

#include "setformulabasic.h"
#include "task.h"

#include <memory>
#include "cuddObj.hh"
#include "unordered_map"

class ExplicitUtil;

class SetFormulaExplicit : public SetFormulaBasic
{
    friend class ExplicitUtil;
private:
    static std::unique_ptr<ExplicitUtil> util;
    BDD set;
    SetFormulaExplicit(BDD bdd);
public:
    SetFormulaExplicit();
    SetFormulaExplicit(std::ifstream &input, Task *task);
    virtual ~SetFormulaExplicit() {}

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

    bool contains(const Cube &statecube);
};

class ExplicitUtil {
    friend class SetFormulaExplicit;
private:
    Task *task;
    std::vector<int> prime_permutation;
    SetFormulaExplicit emptyformula;
    SetFormulaExplicit initformula;
    SetFormulaExplicit goalformula;
    std::vector<BDD> actionformulas;
    std::vector<std::vector<int>> hex;
    std::unordered_map<std::string, BDD> statebdds;

    ExplicitUtil(Task *task);

    BDD build_bdd_from_cube(const Cube &cube);
    BDD build_bdd_for_action(const Action &a);
    void build_actionformulas();

    Cube parseCube(const std::string &param, int size);
    void add_state_to_bdd(BDD &bdd, std::string state);
};

#endif // SETFORMULAEXPLICIT_H
