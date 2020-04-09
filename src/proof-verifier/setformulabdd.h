#ifndef SETFORMULABDD_H
#define SETFORMULABDD_H

#include "stateset.h"
#include "setformulaconstant.h"

#include <unordered_map>
#include <sstream>
#include <memory>
#include <stdio.h>
#include "cuddObj.hh"

class BDDUtil;

struct VectorHasher {
    int operator()(const std::vector<int> &V) const {
        int hash=0;
        for(int i=0;i<V.size();i++) {
            hash+=V[i]; // Can be anything
        }
        return hash;
    }
};

class BDDFile {
private:
    static std::unordered_map<std::vector<int>, BDDUtil, VectorHasher> utils;
    static std::vector<int> compose;
    BDDUtil *util;
    FILE *fp;
    std::unordered_map<int, DdNode *> ddnodes;
public:
    BDDFile() {}
    BDDFile(Task &task, std::string filename);
    // expects caller to take ownership and call deref!
    DdNode *get_ddnode(int index);
    BDDUtil *get_util();
};

struct BDDAction {
    BDD pre;
    BDD eff;
};

class SetFormulaBDD : public StateSetVariable
{
    friend class BDDUtil;
private:
    static std::unordered_map<std::string, BDDFile> bddfiles;
    static std::vector<int> prime_permutation;
    // TODO: can we change this to a reference?
    BDDUtil *util;
    BDD bdd;

    SetFormulaBDD(BDDUtil *util, BDD bdd);
public:
    SetFormulaBDD() = delete;
    SetFormulaBDD(std::stringstream &input, Task &task);

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

    virtual StateSetVariable *get_constant_formula(SetFormulaConstant *c_formula);
    virtual const std::vector<int> &get_varorder();

    // used for checking statement B1 when the left side is an explicit SetFormula
    bool contains(const Cube &statecube) const;

    virtual bool supports_mo() { return true; }
    virtual bool supports_ce() { return true; }
    virtual bool supports_im() { return true; }
    virtual bool supports_me() { return true; }
    virtual bool supports_todnf() { return false; }
    virtual bool supports_tocnf() { return false; }
    virtual bool supports_ct() { return true; }
    virtual bool is_nonsuccint() { return false; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const ;
    virtual bool is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant);
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause);
    virtual bool get_clause(int i, std::vector<int> &varorder, std::vector<bool> &clause);
    virtual int get_model_count();

};

/*
 * For each BDD file, a BDDUtil instance will be created
 * which stores the variable order, all BDDs declared in the file
 * as well as all constant and action formulas in the respective
 * variable order.
 *
 * Note: action formulas are only created on demand (when the method
 * pro/regression_is_union_subset is used the first time by some
 * SetFormulaBDD using this particual BDDUtil)
 */
class BDDUtil {
    friend class SetFormulaBDD;
private:
    Task &task;
    // TODO: fix varorder meaning across the code!
    // global variable i is at position varorder[i](*2)
    std::vector<int> varorder;
    // bdd variable i is global variable other_varorder[i]
    std::vector<int> other_varorder;
    // constant formulas
    SetFormulaBDD emptyformula;
    SetFormulaBDD initformula;
    SetFormulaBDD goalformula;
    std::vector<BDDAction> actionformulas;

    BDD build_bdd_from_cube(const Cube &cube);
    //BDD build_bdd_for_action(const Action &a);
    void build_actionformulas();

    /*
     * Returns a vector containing all BDDs contained in the vector of SetFormulas.
     * The SetFormulas can only be of type SetFormulaBDD or SetFormulaConstant.
     * All SetFormulaBDDs must share the same variable order.
     * If either of these requirements is not satisfied, the method aborts and returns false.
     */
    bool get_bdd_vector(std::vector<StateSetVariable *> &formulas, std::vector<BDD *> &bdds);
public:
    BDDUtil() = delete;
    BDDUtil(Task &task, std::vector<int> &varorder);
};


#endif // SETFORMULABDD_H
