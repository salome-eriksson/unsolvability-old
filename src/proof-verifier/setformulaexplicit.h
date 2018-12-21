#ifndef SETFORMULAEXPLICIT_H
#define SETFORMULAEXPLICIT_H

#include "setformulabasic.h"
#include "task.h"

#include "cuddObj.hh"
#include <memory>
#include <unordered_set>

class ExplicitUtil;
typedef std::vector<std::vector<bool>> ModelExtensions;
typedef std::vector<const std::vector<bool> *>GlobalModel;
typedef std::pair<int,int> GlobalModelVarOcc;

class SetFormulaExplicit : public SetFormulaBasic
{
    friend class ExplicitUtil;
private:
    static std::unique_ptr<ExplicitUtil> util;
    std::vector<int> vars;
    std::unordered_set<std::vector<bool>> models;
    SetFormulaExplicit(std::vector<int> &&vars, std::unordered_set<std::vector<bool>> &&models);
    SetFormulaExplicit(std::vector<const SetFormulaExplicit &> &conjuncts);

    struct OtherFormula {
        SetFormulaExplicit *formula;
        // tells for each var in formula->vars, where in the extended model this var is
        std::vector<GlobalModelVarOcc> var_occurences;
        // positions of vars that first occured in this formula
        std::vector<int> newvars_pos;

        OtherFormula(SetFormulaExplicit *f) {
            formula = f;
            var_occurences.reserve(formula->vars.size());
        }
    };
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

    // model is expected to have the same varorder (ie we need no transformation).
    bool contains(const std::vector<bool> &model) const;
    /*
     * model is expected to have the same varorder (ie we need no transformation).
     * However, the values at position missing_vars can be discarded.
     * Instead we need to consider each combination of the missing vars.
     * Returns those combinations where the (filled out) model is contained.
     */
    std::vector<std::vector<bool>> get_missing_var_values(
            std::vector<bool> &model, const std::vector<int> &missing_vars_pos) const;
};

class ExplicitUtil {
    friend class SetFormulaExplicit;
private:
    Task *task;
    SetFormulaExplicit emptyformula;
    SetFormulaExplicit trueformula;
    SetFormulaExplicit initformula;
    SetFormulaExplicit goalformula;
    std::vector<BDD> actionformulas;
    std::vector<std::vector<bool>> hex;

    ExplicitUtil(Task *task);

    bool get_explicit_vector(std::vector<SetFormula *> &formulas,
                             std::vector<SetFormulaExplicit *> &explicit_formulas);

};

#endif // SETFORMULAEXPLICIT_H
