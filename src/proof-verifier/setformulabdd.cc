#include "setformulabdd.h"

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

BDD BDDUtil::build_bdd_for_action(const Action &a) {
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
}

void BDDUtil::build_actionformulas() {
    actionformulas.reserve(task->get_number_of_actions());
    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        actionformulas.push_back(build_bdd_for_action(task->get_action(i)));
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

    // prog[A] \cap left \subseteq right' iff prog[A] \subseteq !left \cup right'
    BDD neg_left_or_right_primed = !left_singular + right_singular.Permute(&prime_permutation[0]);

    BDD prog_singular = manager.bddOne();
    for (size_t i = 0; i < prog_bdds.size(); ++i) {
        prog_singular *= prog_bdds[i];
    }
    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int action : actions) {
        if (!( (prog_singular*util->actionformulas[action]).Leq(neg_left_or_right_primed) )) {
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

    // prog[A] \cap left \subseteq right' iff prog[A] \subseteq !left \cup right'
    BDD neg_left_or_right_primed = !left_singular + right_singular.Permute(&prime_permutation[0]);

    BDD reg_singular = manager.bddOne();
    for (size_t i = 0; i < reg_bdds.size(); ++i) {
        reg_singular *= reg_bdds[i];
    }

    for (int action : actions) {
        if (!(reg_singular*util->actionformulas[action]).Leq(neg_left_or_right_primed)) {
            return false;
        }
    }
    return true;
}


bool SetFormulaBDD::is_subset_of(SetFormula */*superset*/, bool /*left_positive*/, bool /*right_positive*/) {
    // TODO:implement
    return true;
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

bool SetFormulaBDD::contains(const Cube &statecube) const {
    return util->build_bdd_from_cube(statecube).Leq(bdd);
}
