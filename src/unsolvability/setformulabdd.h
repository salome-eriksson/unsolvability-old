#ifndef SETFORMULABDD_H
#define SETFORMULABDD_H

#include "setformulabasic.h"

#include <unordered_map>
#include <sstream>
#include "cuddObj.hh"

class SetFormulaBDD;

class BDDUtil {
    friend class SetFormulaBDD;
private:
    Task *task;
    Cudd manager;
    std::vector<int> varorder;
    SetFormulaBDD *emptyformula;
    SetFormulaBDD *initformula;
    SetFormulaBDD *goalformula;
    std::vector<BDD> actionformulas;
    std::vector<BDD> bdds;
    BDDUtil(Task *task, std::string filename);

    BDD *build_bdd_from_cube(const Cube &cube);
    BDD build_bdd_for_action(const Action &a);
    void build_actionformulas();

    BDD *get_bdd(int index);
public:
    BDDUtil();
};

class SetFormulaBDD : public SetFormulaBasic
{
    friend class BDDUtil;
private:
    static std::unordered_map<std::string, BDDUtil> utils;
    static std::vector<int> prime_permutation;
    BDDUtil *util;
    BDD *bdd;

    SetFormulaBDD(BDDUtil *util, BDD *bdd);
public:
    SetFormulaBDD(std::ifstream &input, Task *task);
    ~SetFormulaBDD();

    virtual bool is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool is_subset(SetFormula *f1, SetFormula *f2);
    virtual bool intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated);
    virtual bool progression_is_union_subset(SetFormula *f, bool f_negated);
    virtual bool regression_is_union_subset(SetFormula *f, bool f_negated);

    virtual SetFormulaType get_formula_type();
    virtual SetFormulaBasic *get_constant_formula(SetFormulaConstant *c_formula);

    bool contains(const Cube &statecube) const;
};

#endif // SETFORMULABDD_H
