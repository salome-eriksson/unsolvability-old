#ifndef SETFORMULAHORN_H
#define SETFORMULAHORN_H

#include <unordered_map>

#include "setformulabasic.h"
#include "task.h"

class SetFormulaHorn;

class SpecialFormulasHorn {
    friend class SetFormulaHorn;
private:
    static std::unordered_map<ConstantType, SetFormulaHorn *> constantformulas;
    static std::vector<SetFormulaHorn *> actionformulas;
    SpecialFormulasHorn(Task *task);
};


class SetFormulaHorn : public SetFormulaBasic
{
private:
    static SpecialFormulasHorn *specialformulas;
public:
    SetFormulaHorn();

    virtual bool is_subset(SetFormula *f);
    virtual bool is_subset(SetFormula *f1, SetFormula *f2);
    virtual bool intersection_with_goal_is_subset(SetFormula *f);
    virtual bool progression_is_union_subset(SetFormula *f);
    virtual bool regression_is_union_subset(SetFormula *f);

    SetFormulaType get_formula_type();
    virtual SetFormulaBasic *getConstantFormula(ConstantType c);
};

#endif // SETFORMULAHORN_H
