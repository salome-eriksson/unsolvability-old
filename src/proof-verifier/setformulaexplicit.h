#ifndef SETFORMULAEXPLICIT_H
#define SETFORMULAEXPLICIT_H

#include "setformulabasic.h"
#include "task.h"

#include "cuddObj.hh"
#include <memory>
#include <unordered_set>

class ExplicitUtil;
typedef std::vector<bool> Model;
typedef std::vector<Model> ModelExtensions;
typedef std::vector<const Model *>GlobalModel;
typedef std::pair<int,int> GlobalModelVarOcc;

class SetFormulaExplicit : public SetFormulaBasic
{
    friend class ExplicitUtil;
private:
    static std::unique_ptr<ExplicitUtil> util;
    std::vector<int> vars;
    std::unordered_set<Model> models;
    std::unordered_set<Model>::iterator model_it;
    SetFormulaExplicit(std::vector<int> &&vars, std::unordered_set<Model> &&models);
    SetFormulaExplicit(std::vector<SetFormulaExplicit *> &conjuncts);
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
    virtual const std::vector<int> &get_varorder();

    virtual bool supports_implicant_check() { return true; }
    virtual bool supports_clausal_entailment_check() { return true; }
    virtual bool supports_dnf_enumeration() { return true; }
    virtual bool supports_cnf_enumeration() { return false; }
    virtual bool supports_model_enumeration() { return true; }
    virtual bool supports_model_counting() { return true; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const ;
    virtual bool is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant);
    virtual bool get_next_clause(int i, std::vector<int> &vars, std::vector<bool> &clause);
    virtual bool get_next_model(int i, std::vector<bool> &model);
    virtual void setup_model_enumeration();

    /*
     * model is expected to have the same varorder (ie we need no transformation).
     * However, the values at position missing_vars can be discarded.
     * Instead we need to consider each combination of the missing vars.
     * Returns those combinations where the (filled out) model is contained.
     */
    std::vector<Model> get_missing_var_values(
            Model &model, const std::vector<int> &missing_vars_pos) const;
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

    struct OtherVarorderFormula {
        SetFormulaExplicit *formula;
        // tells for each var in formula->vars, where in the extended model this var is
        std::vector<GlobalModelVarOcc> var_occurences;
        // positions of vars that first occured in this formula
        std::vector<int> newvars_pos;

        OtherVarorderFormula(SetFormulaExplicit *f) {
            formula = f;
            var_occurences.reserve(formula->vars.size());
        }
    };
    /*
     * The global model is over the union of vars occuring in any formula.
     * The first vector<bool> covers the vars of left[0], then each entry i until size-1
     * covers the variables newly introduced in other_left_formulas[i+1]
     * (if other_left_formulas[i] does not contain any new vars, the vector is empt<).
     * The last entry covers the variables occuring on the right side.
     * Note that all formulas on the right side must contain the same vars.
     * Also note that the global_model might point to deleted vectors, but those should never be accessed.
     */
    struct SubsetCheckHelper {
        std::vector<int> varorder;
        std::vector<SetFormulaExplicit *> same_varorder_left;
        std::vector<SetFormulaExplicit *> same_varorder_right;
        std::vector<OtherVarorderFormula> other_varorder_left;
        std::vector<OtherVarorderFormula> other_varorder_right;
        GlobalModel global_model;
        std::vector<ModelExtensions> model_extensions;
    };

    bool get_explicit_vector(std::vector<SetFormula *> &formulas,
                             std::vector<SetFormulaExplicit *> &explicit_formulas);
    bool check_same_vars(std::vector<SetFormulaExplicit *> &formulas);
    SubsetCheckHelper get_subset_checker_helper(std::vector<int> &varorder,
                                                std::vector<SetFormulaExplicit *> &left_formulas,
                                                std::vector<SetFormulaExplicit *> &right_formulas);
    bool is_model_contained(const Model &model, SubsetCheckHelper &helper);
};

#endif // SETFORMULAEXPLICIT_H
