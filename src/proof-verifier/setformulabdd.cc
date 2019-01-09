#include "setformulabdd.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include "dddmp.h"

#include "global_funcs.h"
#include "setformulahorn.h"
#include "setformulaexplicit.h"


/*
 * Reads in all BDDs from a given file and stores them
 * in a DdNode* array
 */
BDDUtil::BDDUtil(Task *task, std::string filename)
    : task(task) {

    /*
     * The dumped BDDs only contain the original variables.
     * Since we need also primed variables for checking
     * statements B4 and B5 (pro/regression), we move the
     * variables in such a way that a primed variable always
     * occurs directly after its unprimed version
     * (Example: BDD dump with vars "a b c": "a a' b b' c c'")
     */
    int compose[task->get_number_of_facts()];
    for(int i = 0; i < task->get_number_of_facts(); ++i) {
        compose[i] = 2*i;
    }

    // we need to read in the file as FILE* since dddmp uses this
    FILE *fp;
    fp = fopen(filename.c_str(), "r");
    if(!fp) {
        std::cerr << "could not open bdd file " << filename << std::endl;
    }

    // the first line contains the variable order, separated by space
    varorder.reserve(task->get_number_of_facts());
    // read in line char by char into a stringstream
    std::stringstream ss;
    char c;
    while((c = fgetc (fp)) != '\n') {
        ss << c;
    }
    // read out the same line from the string stream int by int
    // TODO: what happens if it cannot interpret the next word as int?
    // TODO: can we do this more directly? Ie. get the ints while reading the first line.
    int n;
    while (ss >> n){
        varorder.push_back(n);
    }
    assert(varorder.size() == task->get_number_of_facts());
    other_varorder.resize(varorder.size(),-1);
    for (size_t i = 0; i < varorder.size(); ++i) {
        other_varorder[varorder[i]] = i;
    }

    DdNode **tmp_array;
    /* read in the BDDs into an array of DdNodes. The parameters are as follows:
     *  - manager
     *  - how to match roots: we want them to be matched by id
     *  - root names: only needed when you want to match roots by name
     *  - how to match variables: since we want to permute the BDDs in order to
     *    allow primed variables in between the original one, we take COMPOSEIDS
     *  - varnames: needed if you want to match vars according to names
     *  - varmatchauxids: needed if you want to match vars according to auxidc
     *  - varcomposeids: the variable permutation if you want to permute the BDDs
     *  - mode: if the file was dumped in text or in binary mode
     *  - filename: needed if you don't directly pass the FILE *
     *  - FILE*
     *  - Pointer to array where the DdNodes should be saved to
     */
    int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
        DDDMP_VAR_COMPOSEIDS,NULL,NULL,&compose[0],DDDMP_MODE_TEXT,NULL,fp,&tmp_array);

    // we use a vector rather than c-style array for ease of reading
    dd_nodes.reserve(nRoots);
    for(int i = 0; i < nRoots; ++i) {
        dd_nodes.push_back(tmp_array[i]);
    }
    // we do not need to free the memory for tmp_array, memcheck detected no leaks

    initformula = SetFormulaBDD( this, build_bdd_from_cube(task->get_initial_state()) );
    goalformula = SetFormulaBDD( this, build_bdd_from_cube(task->get_goal()) );
    emptyformula = SetFormulaBDD( this, BDD(manager.bddZero()) );
}

BDDUtil::BDDUtil() {

}

// TODO: returning a new object is not a very safe way (see for example contains())
BDD BDDUtil::build_bdd_from_cube(const Cube &cube) {
    std::vector<int> local_cube(cube.size()*2,2);
    for(size_t i = 0; i < cube.size(); ++i) {
        //permute both accounting for primed variables and changed var order
        local_cube[varorder[i]*2] = cube[i];
    }
    return BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &local_cube[0]));
}

/*BDD BDDUtil::build_bdd_for_action(const Action &a) {
    BDD ret = manager.bddOne();
    for(size_t i = 0; i < a.pre.size(); ++i) {
        ret = ret * manager.bddVar(varorder[a.pre[i]]*2);
    }

    //+1 represents primed variable
    for(size_t i = 0; i < a.change.size(); ++i) {
        int permutated_i = varorder[i];
        //add effect
        if(a.change[i] == 1) {
            ret = ret * manager.bddVar(permutated_i*2+1);
        //delete effect
        } else if(a.change[i] == -1) {
            ret = ret - manager.bddVar(permutated_i*2+1);
        //no change -> frame axiom
        } else {
            assert(a.change[i] == 0);
            ret = ret * (manager.bddVar(permutated_i*2) + !manager.bddVar(permutated_i*2+1));
            ret = ret * (!manager.bddVar(permutated_i*2) + manager.bddVar(permutated_i*2+1));
        }
    }
    return ret;
}*/

void BDDUtil::build_actionformulas() {
    actionformulas.reserve(task->get_number_of_actions());
    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        BDDAction bddaction;
        const Action &action = task->get_action(i);
        bddaction.pre = manager.bddOne();
        for (int var : action.pre) {
            bddaction.pre *= manager.bddVar(varorder[var]*2);
        }
        bddaction.eff = manager.bddOne();
        for (size_t var = 0; var < action.change.size(); ++var) {
            if (action.change[var] == 0) {
                continue;
            } else if (action.change[var] == 1) {
                bddaction.eff *= manager.bddVar(varorder[var]*2);
            } else {
                bddaction.eff *= !manager.bddVar(varorder[var]*2);
            }
        }
        actionformulas.push_back(bddaction);
    }
}

bool BDDUtil::get_bdd_vector(std::vector<SetFormula *> &formulas, std::vector<BDD> &bdds) {
    assert(bdds.empty());
    bdds.reserve(formulas.size());
    std::vector<int> *varorder = nullptr;
    for(size_t i = 0; i < formulas.size(); ++i) {
        if (formulas[i]->get_formula_type() == SetFormulaType::CONSTANT) {
            SetFormulaConstant *c_formula = static_cast<SetFormulaConstant *>(formulas[i]);
            // see if we can use get_constant_formula
            switch (c_formula->get_constant_type()) {
            case ConstantType::EMPTY:
                bdds.push_back(emptyformula.bdd);
                break;
            case ConstantType::GOAL:
                bdds.push_back(goalformula.bdd);
                break;
            case ConstantType::INIT:
                bdds.push_back(initformula.bdd);
                break;
            default:
                std::cerr << "Unknown constant type " << std::endl;
                exit_with(ExitCode::CRITICAL_ERROR);
                break;
            }
        } else if(formulas[i]->get_formula_type() == SetFormulaType::BDD) {
            SetFormulaBDD *b_formula = static_cast<SetFormulaBDD *>(formulas[i]);
            if (varorder && b_formula->util->varorder != *varorder) {
                std::cerr << "Error: Trying to combine BDDs with different varorder."
                          << std::endl;
                return false;
            }
            bdds.push_back(b_formula->bdd);
        } else {
            std::cerr << "Error: SetFormula of type other than BDD not allowed here."
                      << std::endl;
            return false;
        }
    }
    return true;
}

std::unordered_map<std::string, BDDUtil> SetFormulaBDD::utils;
std::vector<int> SetFormulaBDD::prime_permutation;

SetFormulaBDD::SetFormulaBDD()
    : util(nullptr), bdd(manager.bddZero()) {

}

SetFormulaBDD::SetFormulaBDD(BDDUtil *util, BDD bdd)
    : util(util), bdd(bdd) {

}

SetFormulaBDD::SetFormulaBDD(std::ifstream &input, Task *task) {
    std::string filename;
    int bdd_index;
    input >> filename;
    input >> bdd_index;
    if(utils.find(filename) == utils.end()) {
        if(utils.empty()) {
            prime_permutation.resize(task->get_number_of_facts()*2, -1);
            for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
              prime_permutation[2*i] = (2*i)+1;
              prime_permutation[(2*i)+1] = 2*i;
            }
        }
        // TODO: is emplace the best thing to do here?
        utils.emplace(std::piecewise_construct, std::make_tuple(filename),
                      std::make_tuple(task, filename));
    }
    util = &(utils[filename]);

    assert(bdd_index >= 0 && bdd_index < util->dd_nodes.size());
    bdd = BDD(manager, util->dd_nodes[bdd_index]);
    Cudd_RecursiveDeref(manager.getManager(), util->dd_nodes[bdd_index]);

    std::string declaration_end;
    input >> declaration_end;
    assert(declaration_end == ";");
}

bool SetFormulaBDD::is_subset(std::vector<SetFormula *> &left,
                              std::vector<SetFormula *> &right) {
    std::vector<BDD> left_bdds;
    std::vector<BDD> right_bdds;
    bool valid_bdds = util->get_bdd_vector(left, left_bdds)
            && util->get_bdd_vector(right, right_bdds);
    if(!valid_bdds) {
        return false;
    }

    BDD left_singular = manager.bddOne();
    for(size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= left_bdds[i];
    }
    BDD right_singular = manager.bddZero();
    for(size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += right_bdds[i];
    }
    return left_singular.Leq(right_singular);
}

bool SetFormulaBDD::is_subset_with_progression(std::vector<SetFormula *> &left,
                                               std::vector<SetFormula *> &right,
                                               std::vector<SetFormula *> &prog,
                                               std::unordered_set<int> &actions) {
    std::vector<BDD> left_bdds;
    std::vector<BDD> right_bdds;
    std::vector<BDD> prog_bdds;
    bool valid_bdds = util->get_bdd_vector(left, left_bdds)
            && util->get_bdd_vector(right, right_bdds)
            && util->get_bdd_vector(prog, prog_bdds);
    if (!valid_bdds) {
        return false;
    }

    BDD left_singular = manager.bddOne();
    for (size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= left_bdds[i];
    }
    BDD right_singular = manager.bddZero();
    for (size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += right_bdds[i];
    }

    BDD neg_left_or_right = (!left_singular) + right_singular;

    BDD prog_singular = manager.bddOne();
    for (size_t i = 0; i < prog_bdds.size(); ++i) {
        prog_singular *= prog_bdds[i];
    }
    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int a : actions) {
        const Action &action = util->task->get_action(a);
        BDD left_rn = left_singular * util->actionformulas[a].pre;
        for (int var = 0; var < util->task->get_number_of_facts(); ++var) {
            if (action.change[var] != 0) {
                prime_permutation[2*var] = 2*var+1;
                prime_permutation[2*var+1] = 2*var;
            } else {
                prime_permutation[2*var] = 2*var;
                prime_permutation[2*var+1] = 2*var+1;
            }
        }
        left_rn.Permute(&prime_permutation[0]);
        if (!( (left_rn*util->actionformulas[a].eff).Leq(neg_left_or_right) )) {
            return false;
        }
    }
    return true;
}

bool SetFormulaBDD::is_subset_with_regression(std::vector<SetFormula *> &left,
                                              std::vector<SetFormula *> &right,
                                              std::vector<SetFormula *> &reg,
                                              std::unordered_set<int> &actions) {
    std::vector<BDD> left_bdds;
    std::vector<BDD> right_bdds;
    std::vector<BDD> reg_bdds;
    bool valid_bdds = util->get_bdd_vector(left, left_bdds)
            && util->get_bdd_vector(right, right_bdds)
            && util->get_bdd_vector(reg, reg_bdds);
    if (!valid_bdds) {
        return false;
    }

    BDD left_singular = manager.bddOne();
    for (size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= left_bdds[i];
    }
    BDD right_singular = manager.bddZero();
    for (size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += right_bdds[i];
    }

    BDD neg_left_or_right = (!left_singular) + right_singular;

    BDD reg_singular = manager.bddOne();
    for (size_t i = 0; i < reg_bdds.size(); ++i) {
        reg_singular *= reg_bdds[i];
    }
    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int a : actions) {
        const Action &action = util->task->get_action(a);
        BDD left_rn = left_singular * util->actionformulas[a].eff;
        for (int var = 0; var < util->task->get_number_of_facts(); ++var) {
            if (action.change[var] != 0) {
                prime_permutation[2*var] = 2*var+1;
                prime_permutation[2*var+1] = 2*var;
            } else {
                prime_permutation[2*var] = 2*var;
                prime_permutation[2*var+1] = 2*var+1;
            }
        }
        left_rn.Permute(&prime_permutation[0]);
        if (!( (left_rn*util->actionformulas[a].pre).Leq(neg_left_or_right) )) {
            return false;
        }
    }
    return true;
}


bool SetFormulaBDD::is_subset_of(SetFormula *superset, bool left_positive, bool right_positive) {
    if (!left_positive || !right_positive) {
        std::cerr << "TODO: not implemented yet!" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    if (bdd.IsZero()) {
        return true;
    }

    // check if each clause in cnf implied by bdd
    if (superset->supports_cnf_enumeration()) {
        int count = 0;
        std::vector<int> varorder;
        std::vector<bool> clause;
        while (superset->get_next_clause(count, varorder, clause)) {
            BDD clause_bdd = manager.bddZero();
            for (size_t i = 0; i < clause.size(); ++i) {
                if (varorder[i] > util->varorder.size()) {
                    continue;
                }
                int bdd_var = 2*util->varorder[varorder[i]];
                if (clause[i]) {
                    clause_bdd += manager.bddVar(bdd_var);
                } else {
                    clause_bdd += !(manager.bddVar(bdd_var));
                }
            }
            if(!bdd.Leq(clause_bdd)) {
                return false;
            }
            count++;
        }
        return true;

    // enumerate models (only works with Explicit if Explicit has all vars this has)
    } else if (superset->get_formula_type() == SetFormulaType::EXPLICIT){
        const std::vector<int> sup_varorder = superset->get_varorder();
        std::vector<bool> model(sup_varorder.size());
        std::vector<int> var_transform(util->varorder.size(), -1);
        std::vector<int> vars_to_fill_base;
        for (size_t i = 0; i < util->varorder.size(); ++i) {
            auto pos_it = std::find(sup_varorder.begin(), sup_varorder.end(), i);
            if (pos_it == sup_varorder.end()) {
                std::cerr << "mixed representation subset check not possible" << std::endl;
                return false;
            }
            var_transform[i] = std::distance(sup_varorder.begin(), pos_it);
        }
        for (size_t i = 0; i < sup_varorder.size(); ++i) {
            if(sup_varorder[i] >= util->varorder.size()) {
                vars_to_fill_base.push_back(i);
            }
        }

        //loop over all models of the BDD
        int* bdd_model;
        CUDD_VALUE_TYPE value_type;
        DdGen *cubegen = Cudd_FirstCube(manager.getManager(),bdd.getNode(),&bdd_model, &value_type);
        // TODO: can the models contain don't cares?
        // Since we checked for ZeroBDD above we will always have at least 1 cube.
        do{
            std::vector<int> vars_to_fill(vars_to_fill_base);
            for (size_t i = 0; i < util->varorder.size(); ++i) {
                // TODO: implicit transformation with primed
                int cube_val = bdd_model[2*util->varorder[i]];
                if (cube_val == 1) {
                    model[var_transform[i]] = true;
                } else  if (cube_val == 0) {
                    model[var_transform[i]] = false;
                } else {
                    vars_to_fill.push_back(i);
                }
            }

            for (int count = 0; count < (1 << vars_to_fill.size()); ++count) {
                for (size_t i = 0; i < vars_to_fill.size(); ++i) {
                    model[vars_to_fill[i]] = ((count >> i) % 2 == 1);
                }
                if (!superset->is_contained(model)) {
                    return false;
                }
            }
        } while(Cudd_NextCube(cubegen,&bdd_model,&value_type) != 0);
        Cudd_GenFree(cubegen);
        return true;
    } else {
        std::cerr << "mixed representation subset check not possible" << std::endl;
        return false;
    }
    return false;
}


SetFormulaType SetFormulaBDD::get_formula_type() {
    return SetFormulaType::BDD;
}

SetFormulaBasic *SetFormulaBDD::get_constant_formula(SetFormulaConstant *c_formula) {
    switch(c_formula->get_constant_type()) {
    case ConstantType::EMPTY:
        return &(util->emptyformula);
        break;
    case ConstantType::INIT:
        return &(util->initformula);
        break;
    case ConstantType::GOAL:
        return &(util->goalformula);
        break;
    default:
        std::cerr << "Unknown Constant type: " << std::endl;
        return nullptr;
        break;
    }
}

const std::vector<int> &SetFormulaBDD::get_varorder() {
    return util->other_varorder;
}

bool SetFormulaBDD::contains(const Cube &statecube) const {
    return util->build_bdd_from_cube(statecube).Leq(bdd);
}

bool SetFormulaBDD::is_contained(const std::vector<bool> &model) const {
    assert(model.size() == util->varorder.size());
    Cube cube(util->varorder.size()*2);
    for (size_t i = 0; i < model.size(); ++i) {
        if(model[i]) {
            cube[2*i] = 1;
        } else {
            cube[2*i] = 0;
        }
    }
    BDD model_bdd(manager, Cudd_CubeArrayToBdd(manager.getManager(), &cube[0]));
    return model_bdd.Leq(bdd);

}

bool SetFormulaBDD::is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant) {
    assert(vars.size() == implicant.size());
    Cube cube(util->varorder.size(), 2);
    for (size_t i = 0; i < vars.size(); ++i) {
        int var = vars[i];
        auto pos_it = std::find(util->varorder.begin(), util->varorder.end(), var);
        if(pos_it != util->varorder.end()) {
            int var_pos = std::distance(util->varorder.begin(), pos_it);
            if (implicant[i]) {
                cube[var_pos] = 1;
            } else {
                cube[var_pos] = 0;
            }
        }
    }
    return util->build_bdd_from_cube(cube).Leq(bdd);
}

bool SetFormulaBDD::get_next_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) {
    return false;
}

bool SetFormulaBDD::get_next_model(int i, std::vector<bool> &model) {
    std::cerr << "not implemented";
    exit_with(ExitCode::CRITICAL_ERROR);
}

void SetFormulaBDD::setup_model_enumeration() {
    std::cerr << "not implemented";
    exit_with(ExitCode::CRITICAL_ERROR);

}
