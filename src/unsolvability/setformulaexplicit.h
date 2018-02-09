#ifndef SETFORMULAEXPLICIT_H
#define SETFORMULAEXPLICIT_H

#include "setformulabasic.h"
#include "task.h"

#include <memory>
#include "cuddObj.hh"


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

    virtual bool is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool is_subset(SetFormula *f1, SetFormula *f2);
    virtual bool intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool progression_is_union_subset(SetFormula *f, bool f_negated);
    virtual bool regression_is_union_subset(SetFormula *f, bool f_negated);

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

    ExplicitUtil(Task *task);

    BDD build_bdd_from_cube(const Cube &cube);
    BDD build_bdd_for_action(const Action &a);
    void build_actionformulas();

    Cube parseCube(const std::string &param, int size);
};

#endif // SETFORMULAEXPLICIT_H
