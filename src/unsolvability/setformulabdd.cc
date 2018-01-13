#include "setformulabdd.h"

#include <cassert>
#include <fstream>
#include "dddmp.h"

#include "global_funcs.h"
#include "setformulahorn.h"

BDDUtil::BDDUtil() {

}

BDDUtil::BDDUtil(Task *task, std::string filename)
    : manager(Cudd(task->get_number_of_facts()*2,0)) {


    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();

    // move variables so the primed versions are in between
    int compose[task->get_number_of_facts()];
    for(int i = 0; i < task->get_number_of_facts(); ++i) {
        compose[i] = 2*i;
    }

    // read in bdd file
    FILE *fp;
    fp = fopen(filename.c_str(), "r");
    if(!fp) {
        std::cerr << "could not open bdd file" << std::endl;
    }

    // first line contains variable order
    varorder.reserve(task->get_number_of_facts());
    // TODO: what is a good value here?
    char line[500];
    fgets(line, sizeof(line), fp);
    std::stringstream ss(line);
    int n;
    while (ss >> n){
        varorder.push_back(n);
    }
    assert(varorder.size() == task->get_number_of_facts());

    DdNode **tmpArray;
    int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
        DDDMP_VAR_COMPOSEIDS,NULL,NULL,&compose[0],DDDMP_MODE_TEXT,NULL,fp,&tmpArray);

    bdds.reserve(nRoots);
    for (int i=0; i<nRoots; i++) {
        bdds.push_back(BDD(manager,tmpArray[i]));
        Cudd_RecursiveDeref(manager.getManager(), tmpArray[i]);
    }
    FREE(tmpArray);

    // set up special formulas
    actionformulas.reserve(task->get_number_of_actions());
    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        actionformulas.push_back(build_bdd_for_action(task->get_action(i)));
    }

    // insert BDDs for initial state, goal and empty set
    initformula = new SetFormulaBDD(this, build_bdd_from_cube(task->get_initial_state()));
    goalformula = new SetFormulaBDD(this, build_bdd_from_cube(task->get_goal()));
    BDD *emptybdd = new BDD(manager.bddZero());
    emptyformula = new SetFormulaBDD(this, emptybdd);
}

BDD *BDDUtil::build_bdd_from_cube(const Cube &cube) {
    std::vector<int> local_cube(cube.size()*2,2);
    for(size_t i = 0; i < cube.size(); ++i) {
        //permute both accounting for primed variables and changed var order
        local_cube[varorder[i]*2] = cube[i];
    }
    return new BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &local_cube[0]));
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

BDD *BDDUtil::get_bdd(int index) {
    assert(index >= 0 && index < bdds.size());
    return &(bdds[index]);
}

std::unordered_map<std::string, BDDUtil> SetFormulaBDD::utils;
std::vector<int> SetFormulaBDD::prime_permutation;

SetFormulaBDD::SetFormulaBDD(BDDUtil *util, BDD *bdd)
    : util(util), bdd(bdd) {

}

SetFormulaBDD::SetFormulaBDD(std::ifstream &input, Task *task) {
    std::string filename;
    input >> filename;
    int index;
    input >> index;
    if(utils.find(filename) == utils.end()) {
        if(utils.empty()) {
            prime_permutation.resize(task->get_number_of_facts()*2, -1);
            for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
              prime_permutation[2*i] = (2*i)+1;
              prime_permutation[(2*i)+1] = 2*i;
            }
        }
        utils[filename] = BDDUtil(task, filename);
    }
    bdd = utils[filename].get_bdd(index);
    util = &(utils[filename]);
    assert(bdd != nullptr);

    std::string tmp;
    input >> tmp;
    assert(tmp.compare(";") == 0);
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
            BDD tmp = !(util->manager.bddVar(util->varorder[i]*2));
            if(!((*bdd).Leq(tmp))) {
                std::cout << "forced false " << i << std::endl;
                return false;
            }
        }

        for(int i = 0; i < f_horn->get_forced_true().size(); ++i) {
            BDD tmp = (util->manager.bddVar(util->varorder[i]*2));
            if(!((*bdd).Leq(tmp))) {
                std::cout << "forced true " << i << std::endl;
                return false;
            }
        }

        for(int i = 0; i < f_horn->get_size(); ++i) {
            BDD tmp = util->manager.bddZero();
            for(int j : f_horn->get_left_vars(i)) {
                tmp += !(util->manager.bddVar(util->varorder[j]*2));
            }
            if(f_horn->get_right(i) != -1) {
                tmp += (util->manager.bddVar(util->varorder[i]*2));
            }
            if(!((*bdd).Leq(tmp))) {
                std::cout << "clause " << i << std::endl;
                return false;
            }
        }
        return true;
        break;
    }
    case SetFormulaType::BDD: {
        const SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);
        BDD left = *bdd;
        if(negated) {
            left = !left;
        }
        BDD right = *(f_bdd->bdd);
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
    case SetFormulaType::EXPLICIT:
        std::cerr << "not implemented yet";
        return false;
        break;
    default:
        std::cerr << "X \\subseteq X' is not supported for BDD formula X "
                     "and non-basic or constant formula X'" << std::endl;
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
    return bdd->Leq(*(f1_bdd->bdd) + *(f2_bdd->bdd));
}


bool SetFormulaBDD::intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    } else if(f->get_formula_type() != SetFormulaType::BDD) {
        std::cerr << "L \\cap S_G(\\Pi) \\subseteq L' is not supported for BDD Formula L";
        std::cerr <<"and non-BDD formula L'" << std::endl;
        return false;
    }
    const SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);
    BDD left = *bdd;
    if(negated) {
        left = !left;
    }
    left = left * *(util->goalformula->bdd);
    BDD right = *(f_bdd->bdd);
    if(f_negated) {
        right = !right;
    }
}


bool SetFormulaBDD::progression_is_union_subset(SetFormula *f, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    } else if(f->get_formula_type() != SetFormulaType::BDD) {
        std::cerr << "X[a] \\subseteq X \\land L is not supported for BDD Formula X";
        std::cerr <<"and non-BDD formula L" << std::endl;
        return false;
    }
    const SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);
    BDD possible_successors = *(f_bdd->bdd);
    if(f_negated) {
        possible_successors = !possible_successors;
    }
    possible_successors += *(bdd);
    possible_successors = possible_successors.Permute(&prime_permutation[0]);

    for(int i = 0; i < util->actionformulas.size(); ++i) {
        BDD succ = *(bdd) * util->actionformulas[i];
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
        std::cerr << "X[a] \\subseteq X \\land L is not supported for BDD Formula X";
        std::cerr <<"and non-BDD formula L" << std::endl;
        return false;
    }
    const SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);
    BDD possible_predecessors = *(f_bdd->bdd);
    if(f_negated) {
        possible_predecessors = !possible_predecessors;
    }
    possible_predecessors += *(bdd);

    for(int i = 0; i < util->actionformulas.size(); ++i) {
        BDD pred = bdd->Permute(&prime_permutation[0]) * util->actionformulas[i];
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
        return util->emptyformula;
        break;
    case ConstantType::INIT:
        return util->initformula;
        break;
    case ConstantType::GOAL:
        return util->goalformula;
        break;
    default:
        std::cerr << "Unknown Constant type: " << std::endl;
        return nullptr;
        break;
    }
}
