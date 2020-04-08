#include "setformulabdd.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include "dddmp.h"

#include "global_funcs.h"
#include "setformulahorn.h"
#include "setformulaexplicit.h"

std::unordered_map<std::vector<int>, BDDUtil, VectorHasher> BDDFile::utils;
std::vector<int> BDDFile::compose;

BDDFile::BDDFile(Task &task, std::string filename) {
    if (compose.empty()) {
        /*
         * The dumped BDDs only contain the original variables.
         * Since we need also primed variables for checking
         * statements B4 and B5 (pro/regression), we move the
         * variables in such a way that a primed variable always
         * occurs directly after its unprimed version
         * (Example: BDD dump with vars "a b c": "a a' b b' c c'")
         */
        compose.resize(task.get_number_of_facts());
        for(int i = 0; i < task.get_number_of_facts(); ++i) {
            compose[i] = 2*i;
        }

    }
    // we need to read in the file as FILE* since dddmp uses this
    fp = fopen(filename.c_str(), "r");
    if(!fp) {
        std::cerr << "could not open bdd file " << filename << std::endl;
    }

    // the first line contains the variable order, separated by space
    std::vector<int> varorder;
    varorder.reserve(task.get_number_of_facts());
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
    assert(varorder.size() == task.get_number_of_facts());
    auto it = utils.find(varorder);
    if (it == utils.end()) {
        it = utils.emplace(std::piecewise_construct,
                      std::forward_as_tuple(varorder),
                      std::forward_as_tuple(task,varorder)).first;
    }
    util = &(it->second);

}

DdNode *BDDFile::get_ddnode(int index) {
    auto it = ddnodes.find(index);
    if (it != ddnodes.end()) {

    }

    bool found = false;
    if (it != ddnodes.end()) {
        found = true;
    }
    while(!found) {
        std::vector<int> indices;
        // next line contains indices
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
            indices.push_back(n);
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
            DDDMP_VAR_COMPOSEIDS,NULL,NULL,compose.data(),DDDMP_MODE_TEXT,NULL,fp,&tmp_array);

        assert(indices.size() == nRoots);
        for(int i = 0; i < nRoots; ++i) {
            if (indices[i] == index) {
                found = true;
                it = ddnodes.insert(std::make_pair(indices[i],tmp_array[i])).first;
            } else {
                ddnodes.insert(std::make_pair(indices[i],tmp_array[i]));
            }
        }
        // we do not need to free the memory for tmp_array, memcheck detected no leaks
    }
    DdNode *ret = it->second;
    //ddnodes.erase(it);
    return ret;
}

BDDUtil *BDDFile::get_util() {
    return util;
}

BDDUtil::BDDUtil(Task &task, std::vector<int> &varorder)
    : task(task), varorder(varorder),
      initformula(this, build_bdd_from_cube(task.get_initial_state())),
      goalformula(this, build_bdd_from_cube(task.get_goal())),
      emptyformula(this, BDD(manager.bddZero())) {

    assert(varorder.size() == task.get_number_of_facts());
    other_varorder.resize(varorder.size(),-1);
    for (size_t i = 0; i < varorder.size(); ++i) {
        other_varorder[varorder[i]] = i;
    }
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

void BDDUtil::build_actionformulas() {
    actionformulas.reserve(task.get_number_of_actions());
    for(int i = 0; i < task.get_number_of_actions(); ++i) {
        BDDAction bddaction;
        const Action &action = task.get_action(i);
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

bool BDDUtil::get_bdd_vector(std::vector<StateSetVariable *> &formulas, std::vector<BDD *> &bdds) {
    assert(bdds.empty());
    bdds.reserve(formulas.size());
    std::vector<int> *varorder = nullptr;
    // TODO: reimplement check whether formula is horn or constant
    for(size_t i = 0; i < formulas.size(); ++i) {
        SetFormulaConstant *c_formula = dynamic_cast<SetFormulaConstant *>(formulas[i]);
        SetFormulaBDD *b_formula = dynamic_cast<SetFormulaBDD *>(formulas[i]);
        if (c_formula) {
            // see if we can use get_constant_formula
            switch (c_formula->get_constant_type()) {
            case ConstantType::EMPTY:
                bdds.push_back(&(emptyformula.bdd));
                break;
            case ConstantType::GOAL:
                bdds.push_back(&(goalformula.bdd));
                break;
            case ConstantType::INIT:
                bdds.push_back(&(initformula.bdd));
                break;
            default:
                std::cerr << "Unknown constant type " << std::endl;
                exit_with(ExitCode::CRITICAL_ERROR);
                break;
            }
        } else if(b_formula) {
            if (varorder && b_formula->util->varorder != *varorder) {
                std::cerr << "Error: Trying to combine BDDs with different varorder."
                          << std::endl;
                return false;
            }
            bdds.push_back(&(b_formula->bdd));
        } else {
            std::cerr << "Error: SetFormula of type other than BDD not allowed here."
                      << std::endl;
            return false;
        }
    }
    return true;
}

std::unordered_map<std::string, BDDFile> SetFormulaBDD::bddfiles;
std::vector<int> SetFormulaBDD::prime_permutation;

SetFormulaBDD::SetFormulaBDD(BDDUtil *util, BDD bdd)
    : util(util), bdd(bdd) {
}

SetFormulaBDD::SetFormulaBDD(std::stringstream &input, Task &task) {
    std::string filename;
    int bdd_index;
    input >> filename;
    input >> bdd_index;
    if(bddfiles.find(filename) == bddfiles.end()) {
        if(bddfiles.empty()) {
            prime_permutation.resize(task.get_number_of_facts()*2, -1);
            for(int i = 0 ; i < task.get_number_of_facts(); ++i) {
              prime_permutation[2*i] = (2*i)+1;
              prime_permutation[(2*i)+1] = 2*i;
            }
        }
        bddfiles.emplace(std::piecewise_construct,
                         std::forward_as_tuple(filename),
                         std::forward_as_tuple(task, filename));
    }
    BDDFile &bddfile = bddfiles[filename];
    util = bddfile.get_util();

    assert(bdd_index >= 0);
    bdd = BDD(manager, bddfile.get_ddnode(bdd_index));
    Cudd_RecursiveDeref(manager.getManager(), bdd.getNode());


    std::string declaration_end;
    input >> declaration_end;
    assert(declaration_end == ";");
}

bool SetFormulaBDD::check_statement_b1(std::vector<StateSetVariable *> &left,
                                       std::vector<StateSetVariable *> &right) {
    std::vector<BDD *> left_bdds;
    std::vector<BDD *> right_bdds;
    bool valid_bdds = util->get_bdd_vector(left, left_bdds)
            && util->get_bdd_vector(right, right_bdds);
    if(!valid_bdds) {
        return false;
    }

    BDD left_singular = manager.bddOne();
    for(size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= *(left_bdds[i]);
    }
    BDD right_singular = manager.bddZero();
    for(size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += *(right_bdds[i]);
    }
    return left_singular.Leq(right_singular);
}

bool SetFormulaBDD::check_statement_b2(std::vector<StateSetVariable *> &progress,
                                       std::vector<StateSetVariable *> &left,
                                       std::vector<StateSetVariable *> &right,
                                       std::unordered_set<int> &action_indices) {

    std::vector<BDD *> left_bdds;
    std::vector<BDD *> right_bdds;
    std::vector<BDD *> prog_bdds;
    bool valid_bdds = util->get_bdd_vector(left, left_bdds)
            && util->get_bdd_vector(right, right_bdds)
            && util->get_bdd_vector(progress, prog_bdds);
    if (!valid_bdds) {
        return false;
    }

    BDD left_singular = manager.bddOne();
    for (size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= *(left_bdds[i]);
    }
    BDD right_singular = manager.bddZero();
    for (size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += *(right_bdds[i]);
    }

    BDD neg_left_or_right = (!left_singular) + right_singular;

    BDD prog_singular = manager.bddOne();
    for (size_t i = 0; i < prog_bdds.size(); ++i) {
        prog_singular *= *(prog_bdds[i]);
    }
    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int a : action_indices) {
        const Action &action = util->task.get_action(a);
        BDD prog_rn = prog_singular * util->actionformulas[a].pre;

        for (int var = 0; var < util->task.get_number_of_facts(); ++var) {
            int local_var = util->varorder[var];
            if (action.change[var] != 0) {
                prime_permutation[2*local_var] = 2*local_var+1;
                prime_permutation[2*local_var+1] = 2*local_var;
            } else {
                prime_permutation[2*local_var] = 2*local_var;
                prime_permutation[2*local_var+1] = 2*local_var+1;
            }
        }
        prog_rn = (prog_rn.Permute(&prime_permutation[0]));
        if ( !((prog_rn* util->actionformulas[a].eff).Leq(neg_left_or_right)) ) {
            return false;
        }
    }
    return true;
}

bool SetFormulaBDD::check_statement_b3(std::vector<StateSetVariable *> &regress,
                                       std::vector<StateSetVariable *> &left,
                                       std::vector<StateSetVariable *> &right,
                                       std::unordered_set<int> &action_indices) {
    std::vector<BDD *> left_bdds;
    std::vector<BDD *> right_bdds;
    std::vector<BDD *> reg_bdds;
    bool valid_bdds = util->get_bdd_vector(left, left_bdds)
            && util->get_bdd_vector(right, right_bdds)
            && util->get_bdd_vector(regress, reg_bdds);
    if (!valid_bdds) {
        return false;
    }

    BDD left_singular = manager.bddOne();
    for (size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= *(left_bdds[i]);
    }
    BDD right_singular = manager.bddZero();
    for (size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += *(right_bdds[i]);
    }

    BDD neg_left_or_right = (!left_singular) + right_singular;

    BDD reg_singular = manager.bddOne();
    for (size_t i = 0; i < reg_bdds.size(); ++i) {
        reg_singular *= *(reg_bdds[i]);
    }
    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int a : action_indices) {
        const Action &action = util->task.get_action(a);
        BDD reg_rn = reg_singular * util->actionformulas[a].eff;
        for (int var = 0; var < util->task.get_number_of_facts(); ++var) {
            int local_var = util->varorder[var];
            if (action.change[var] != 0) {
                prime_permutation[2*local_var] = 2*local_var+1;
                prime_permutation[2*local_var+1] = 2*local_var;
            } else {
                prime_permutation[2*local_var] = 2*local_var;
                prime_permutation[2*local_var+1] = 2*local_var+1;
            }
        }
        reg_rn = reg_rn.Permute(&prime_permutation[0]);
        if (!( (reg_rn*util->actionformulas[a].pre).Leq(neg_left_or_right) )) {
            return false;
        }
    }
    return true;
}

bool SetFormulaBDD::check_statement_b4(StateSetVariable *right, bool left_positive, bool right_positive) {

    if (!left_positive && !right_positive) {
        return right->check_statement_b4(this, true, true);
    } else if (left_positive && !right_positive) {
        if (right->supports_todnf() || right->get_formula_type() == SetFormulaType::EXPLICIT) {
            return right->check_statement_b4(this, true, false);
        } else {
            std::cerr << "mixed representation subset check not possible" << std::endl;
            return false;
        }
    }

    // right positive

    BDD pos_bdd = bdd;
    if (!left_positive) {
        pos_bdd = !bdd;
    }

    // left positive
    if (pos_bdd.IsZero()) {
        return true;
    }

    if (right->supports_tocnf()) {
        int count = 0;
        std::vector<int> varorder;
        std::vector<bool> clause;
        while (right->get_clause(count, varorder, clause)) {
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
            if(!pos_bdd.Leq(clause_bdd)) {
                return false;
            }
            count++;
        }
        return true;

    // enumerate models (only works with Explicit if Explicit has all vars this has)
    } else if (right->get_formula_type() == SetFormulaType::EXPLICIT) {
        const std::vector<int> sup_varorder = right->get_varorder();
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
        DdGen *cubegen = Cudd_FirstCube(manager.getManager(),pos_bdd.getNode(),&bdd_model, &value_type);
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
                if (!right->is_contained(model)) {
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
}


SetFormulaType SetFormulaBDD::get_formula_type() {
    return SetFormulaType::BDD;
}

StateSetVariable *SetFormulaBDD::get_constant_formula(SetFormulaConstant *c_formula) {
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

bool SetFormulaBDD::is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) {
    assert(varorder.size() == implicant.size());
    Cube cube(util->varorder.size(), 2);
    for (size_t i = 0; i < varorder.size(); ++i) {
        int var = varorder[i];
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

bool SetFormulaBDD::is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) {
    assert(varorder.size() == clause.size());
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
    return bdd.Leq(clause_bdd);
}

bool SetFormulaBDD::get_clause(int i, std::vector<int> &varorder, std::vector<bool> &clause) {
    return false;
}

int SetFormulaBDD::get_model_count() {
    // TODO: not sure if this is correct
    return bdd.CountMinterm(util->varorder.size());
}

StateSetBuilder<SetFormulaBDD> bdd_builder("b");
