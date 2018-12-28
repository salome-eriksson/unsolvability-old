#ifndef SETFORMULACOMPOUND_H
#define SETFORMULACOMPOUND_H

#include "setformula.h"

class SetFormulaCompound : public SetFormula
{
public:
    SetFormulaCompound();
    virtual ~SetFormulaCompound() {}

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

    virtual SetFormulaType get_formula_type() = 0;
    virtual const std::vector<int> &get_varorder();

    virtual bool supports_implicant_check() { return false; }
    virtual bool supports_clausal_entailment_check() { return false; }
    virtual bool supports_dnf_enumeration() { return false; }
    virtual bool supports_cnf_enumeration() { return false; }
    virtual bool supports_model_enumeration() { return false; }
    virtual bool supports_model_counting() { return false; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const;
    virtual bool is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant);
    virtual bool get_next_clause(int i, std::vector<int> &vars, std::vector<bool> &clause);
    virtual bool get_next_model(int i, std::vector<bool> &model);
    virtual void setup_model_enumeration();
};


class SetFormulaNegation : public SetFormulaCompound {
private:
    FormulaIndex subformula_index;
public:
    SetFormulaNegation(FormulaIndex subformula_index);
    virtual ~SetFormulaNegation() {}

    FormulaIndex get_subformula_index();

    virtual SetFormulaType get_formula_type();
};

class SetFormulaIntersection : public SetFormulaCompound {
private:
    FormulaIndex left_index;
    FormulaIndex right_index;
public:
    SetFormulaIntersection(FormulaIndex left_index, FormulaIndex right_index);
    virtual ~SetFormulaIntersection() {}

    FormulaIndex get_left_index();
    FormulaIndex get_right_index();

    virtual SetFormulaType get_formula_type();
};

class SetFormulaUnion : public SetFormulaCompound {
private:
    FormulaIndex left_index;
    FormulaIndex right_index;
public:
    SetFormulaUnion(FormulaIndex left_index, FormulaIndex right_index);
    virtual ~SetFormulaUnion() {}

    FormulaIndex get_left_index();
    FormulaIndex get_right_index();

    virtual SetFormulaType get_formula_type();
};

class SetFormulaProgression : public SetFormulaCompound {
private:
    FormulaIndex subformula_index;
    ActionSetIndex actionset_index;
public:
    SetFormulaProgression(FormulaIndex subformula_index, ActionSetIndex actionset_index);
    virtual ~SetFormulaProgression() {}

    FormulaIndex get_subformula_index();
    ActionSetIndex get_actionset_index();

    virtual SetFormulaType get_formula_type();
};

class SetFormulaRegression : public SetFormulaCompound {
private:
    FormulaIndex subformula_index;
    ActionSetIndex actionset_index;
public:
    SetFormulaRegression(FormulaIndex subformula_index, ActionSetIndex actionset_index);
    virtual ~SetFormulaRegression() {}

    FormulaIndex get_subformula_index();
    ActionSetIndex get_actionset_index();

    virtual SetFormulaType get_formula_type();
};

#endif // SETFORMULACOMPOUND_H
