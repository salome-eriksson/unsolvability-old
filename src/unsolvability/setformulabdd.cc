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

bool SetFormulaBDD::is_subset(SetFormula *f, bool negated, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    }
    switch (f->get_formula_type()) {
    case SetFormulaType::HORN: {
        if(negated || f_negated) {
            std::cerr << "L \\subseteq L' not supported for BDD formula L and ";
            std::cerr << "Horn formula L' if one of them is negated" << std::endl;
            return false;
        }

        const SetFormulaHorn *f_horn = static_cast<SetFormulaHorn *>(f);
        for(int i : f_horn->get_forced_false()) {
            BDD tmp = !(manager.bddVar(util->varorder[i]*2));
            if(!bdd.Leq(tmp)) {
                return false;
            }
        }

        for(int i = 0; i < f_horn->get_forced_true().size(); ++i) {
            BDD tmp = (manager.bddVar(util->varorder[i]*2));
            if(!(bdd.Leq(tmp))) {
                return false;
            }
        }

        for(int i = 0; i < f_horn->get_size(); ++i) {
            BDD tmp = manager.bddZero();
            for(int j : f_horn->get_left_vars(i)) {
                tmp += !(manager.bddVar(util->varorder[j]*2));
            }
            if(f_horn->get_right(i) != -1) {
                tmp += (manager.bddVar(util->varorder[i]*2));
            }
            if(!(bdd.Leq(tmp))) {
                return false;
            }
        }
        return true;
        break;
    }
    // TODO: check for different varorder
    case SetFormulaType::BDD: {
        const SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);

        if(util->varorder != f_bdd->util->varorder) {
            std::cerr << "L \\subseteq L' not supported for BDD formulas L and "
                      << "L' with different variable order." << std::endl;
            return false;
        }

        BDD left = bdd;
        if(negated) {
            left = !left;
        }
        BDD right = f_bdd->bdd;
        if(f_negated) {
            right = !right;
        }
        return left.Leq(right);
        break;
    }
    case SetFormulaType::TWOCNF:
        std::cerr << "not implemented yet";
        return false;
        break;
    case SetFormulaType::EXPLICIT: {
        if(negated || f_negated) {
            std::cerr << "L \\subseteq L' not supported for BDD formula L and "
                      << "explicit formula L' if one of them is negated" << std::endl;
            return false;
        }

        if(bdd.IsZero()) {
            return true;
        }

        SetFormulaExplicit *f_explicit = static_cast<SetFormulaExplicit *>(f);

        int* bdd_model;
        // TODO: find a better way to find out how many variables we have
        Cube statecube(util->varorder.size(),-1);
        CUDD_VALUE_TYPE value_type;
        DdGen *cubegen = Cudd_FirstCube(manager.getManager(),bdd.getNode(),&bdd_model, &value_type);
        // TODO: can the models contain don't cares?
        // Since we checked for ZeroBDD above we will always have at least 1 cube.
        do{
            for(int i = 0; i < statecube.size(); ++i) {
                statecube[i] = bdd_model[2*util->varorder[i]];
            }
            if(!f_explicit->contains(statecube)) {
                std::cout << "SetFormulaExplicit does not contain the following cube:";
                for(int i = 0; i < statecube.size(); ++i) {
                    std::cout << statecube[i] << " ";
                } std::cout << std::endl;
                Cudd_GenFree(cubegen);
                return false;
            }
        } while(Cudd_NextCube(cubegen,&bdd_model,&value_type) != 0);
        Cudd_GenFree(cubegen);
        return true;
        break;
    }
    default:
        std::cerr << "L \\subseteq L' is not supported for BDD formula L "
                     "and non-basic or constant formula L'" << std::endl;
        return false;
        break;
    }
}


bool SetFormulaBDD::is_subset(SetFormula *f1, SetFormula *f2) {
    if(f1->get_formula_type() == SetFormulaType::CONSTANT) {
        f1 = get_constant_formula(static_cast<SetFormulaConstant *>(f1));
    }
    if(f2->get_formula_type() == SetFormulaType::CONSTANT) {
        f2 = get_constant_formula(static_cast<SetFormulaConstant *>(f2));
    }
    if(f1->get_formula_type() != SetFormulaType::BDD || f2->get_formula_type() != SetFormulaType::BDD) {
        std::cerr << "X \\subseteq X' \\cup X'' is not supported for BDD formula X ";
        std::cerr << "and non-BDD formula X' or X''" << std::endl;
        return false;
    }
    const SetFormulaBDD *f1_bdd = static_cast<SetFormulaBDD *>(f1);
    const SetFormulaBDD *f2_bdd = static_cast<SetFormulaBDD *>(f2);

    if(util->varorder != f1_bdd->util->varorder || util->varorder != f2_bdd->util->varorder) {
        std::cerr << "X \\subseteq X' \\cup X'' is not supported for BDD formulas X ";
        std::cerr << "X' and X'' with different variable order." << std::endl;
        return false;
    }

    return bdd.Leq(f1_bdd->bdd + f2_bdd->bdd);
}


bool SetFormulaBDD::intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    } else if(f->get_formula_type() != SetFormulaType::BDD) {
        std::cerr << "L \\cap S_G(\\Pi) \\subseteq L' is not supported for BDD formula L ";
        std::cerr << "and non-BDD formula L'" << std::endl;
        return false;
    }
    const SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);

    if(util->varorder != f_bdd->util->varorder) {
        std::cerr << "L \\cap S_G(\\Pi) \\subseteq L' not supported for BDD formulas L and "
                  << "L' with different variable order." << std::endl;
        return false;
    }

    BDD left = bdd;
    if(negated) {
        left = !left;
    }
    left = left * util->goalformula.bdd;
    BDD right = f_bdd->bdd;
    if(f_negated) {
        right = !right;
    }
    return left.Leq(right);
}


bool SetFormulaBDD::progression_is_union_subset(SetFormula *f, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    } else if(f->get_formula_type() != SetFormulaType::BDD) {
        std::cerr << "X[A] \\subseteq X \\land L is not supported for BDD formula X ";
        std::cerr << "and non-BDD formula L" << std::endl;
        return false;
    }
    const SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);

    if(util->varorder != f_bdd->util->varorder) {
        std::cerr << "X[A] \\subseteq X \\land L is not supported for BDD formulas X and "
                  << "L with different variable order." << std::endl;
        return false;
    }

    BDD possible_successors = f_bdd->bdd;
    if(f_negated) {
        possible_successors = !possible_successors;
    }
    possible_successors += bdd;
    possible_successors = possible_successors.Permute(&prime_permutation[0]);

    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for(int i = 0; i < util->actionformulas.size(); ++i) {
        BDD succ = bdd * util->actionformulas[i];
        if(!succ.Leq(possible_successors)) {
            return false;
        }
    }
    return true;
}


bool SetFormulaBDD::regression_is_union_subset(SetFormula *f, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    } else if(f->get_formula_type() != SetFormulaType::BDD) {
        std::cerr << "[A]X \\subseteq X \\land L is not supported for BDD formula X ";
        std::cerr << "and non-BDD formula L" << std::endl;
        return false;
    }
    const SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);

    if(util->varorder != f_bdd->util->varorder) {
        std::cerr << "[A]X \\subseteq X \\land L is not supported for BDD formulas X and "
                  << "L with different variable order." << std::endl;
        return false;
    }

    BDD possible_predecessors = f_bdd->bdd;
    if(f_negated) {
        possible_predecessors = !possible_predecessors;
    }
    possible_predecessors += bdd;

    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for(int i = 0; i < util->actionformulas.size(); ++i) {
        BDD pred = bdd.Permute(&prime_permutation[0]) * util->actionformulas[i];
        if(!pred.Leq(possible_predecessors)) {
            return false;
        }
    }
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
