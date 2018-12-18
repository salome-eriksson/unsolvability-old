#include "setformulaexplicit.h"
#include "setformulahorn.h"
#include "setformulabdd.h"

#include <cassert>
#include <fstream>
#include <iostream>

#include "global_funcs.h"


ExplicitUtil::ExplicitUtil(Task *task)
    : task(task) {

    // TODO: move this to where the manager is declared
    prime_permutation.resize(task->get_number_of_facts()*2, -1);
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      prime_permutation[2*i] = (2*i)+1;
      prime_permutation[(2*i)+1] = 2*i;
    }

    initformula = SetFormulaExplicit(build_bdd_from_cube(task->get_initial_state()));
    goalformula = SetFormulaExplicit(build_bdd_from_cube(task->get_goal()));
    emptyformula = SetFormulaExplicit(BDD(manager.bddZero()));

    hex.reserve(16);
    for(int i = 0; i < 16; ++i) {
        std::vector<int> tmp = { (i >> 3)%2, (i >> 2)%2, (i >> 1)%2, i%2};
        hex.push_back(tmp);
    }

}

BDD ExplicitUtil::build_bdd_from_cube(const Cube &cube) {
    std::vector<int> local_cube(cube.size()*2,2);
    for(size_t i = 0; i < cube.size(); ++i) {
        // permute accounting for primed variables
        local_cube[i*2] = cube[i];
    }
    return BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &local_cube[0]));
}

BDD ExplicitUtil::build_bdd_for_action(const Action &a) {
    BDD ret = manager.bddOne();
    for(size_t i = 0; i < a.pre.size(); ++i) {
        ret = ret * manager.bddVar(a.pre[i]*2);
    }
    //+1 represents primed variable
    for(size_t i = 0; i < a.change.size(); ++i) {
        //add effect
        if(a.change[i] == 1) {
            ret = ret * manager.bddVar(i*2+1);
        //delete effect
        } else if(a.change[i] == -1) {
            ret = ret - manager.bddVar(i*2+1);
        //no change -> frame axiom
        } else {
            assert(a.change[i] == 0);
            ret = ret * (manager.bddVar(i*2) + !manager.bddVar(i*2+1));
            ret = ret * (!manager.bddVar(i*2) + manager.bddVar(i*2+1));
        }
    }
    return ret;
}

void ExplicitUtil::build_actionformulas() {
    actionformulas.reserve(task->get_number_of_actions());
    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        actionformulas.push_back(build_bdd_for_action(task->get_action(i)));
    }
}

Cube ExplicitUtil::parseCube(const std::string &param, int size) {
    assert(param.length() == (size+3)/4);
    Cube cube;
    cube.reserve(size+3);
    for(int i = 0; i < param.length(); ++i) {
        const std::vector<int> *vec;
        if(param.at(i) < 'a') {
            vec = &hex.at(param.at(i)-'0');
        } else {
            vec = &hex.at(param.at(i)-'a'+10);
        }

        cube.insert(cube.end(), vec->begin(), vec->end());
    }
    cube.resize(size);
    return cube;
}

void ExplicitUtil::add_state_to_bdd(BDD &bdd, std::string state) {
    if(statebdds.find(state) == statebdds.end()) {
        BDD statebdd = build_bdd_from_cube(parseCube(state, task->get_number_of_facts()));
        auto ins = statebdds.insert(std::make_pair(state,statebdd));
        bdd += ins.first->second;
    } else {
        bdd += statebdds[state];
    }
}

std::unique_ptr<ExplicitUtil> SetFormulaExplicit::util;

SetFormulaExplicit::SetFormulaExplicit() {

}

SetFormulaExplicit::SetFormulaExplicit(BDD set)
    : set(set) {

}

SetFormulaExplicit::SetFormulaExplicit(std::ifstream &input, Task *task) {
    if(!util) {
        util = std::unique_ptr<ExplicitUtil>(new ExplicitUtil(task));
    }
    set = manager.bddZero();
    std::string s;
    input >> s;
    while(s.compare(";") != 0) {
        util->add_state_to_bdd(set, s);
        input >> s;
    }
}


bool SetFormulaExplicit::is_subset(std::vector<SetFormula *> &/*left*/,
                                   std::vector<SetFormula *> &/*right*/) {
    // TODO:implement
    return true;
}

bool SetFormulaExplicit::is_subset_with_progression(std::vector<SetFormula *> &/*left*/,
                                                    std::vector<SetFormula *> &/*right*/,
                                                    std::vector<SetFormula *> &/*prog*/,
                                                    std::unordered_set<int> &/*actions*/) {
    // TODO:implement
    return true;
}

bool SetFormulaExplicit::is_subset_with_regression(std::vector<SetFormula *> &/*left*/,
                                                   std::vector<SetFormula *> &/*right*/,
                                                   std::vector<SetFormula *> &/*reg*/,
                                                   std::unordered_set<int> &/*actions*/) {
    // TODO:implement
    return true;
}

bool SetFormulaExplicit::is_subset_of(SetFormula */*superset*/, bool /*left_positive*/, bool right_/*positive*/) {
    // TODO:implement
    return true;
}


SetFormulaType SetFormulaExplicit::get_formula_type() {
    return SetFormulaType::EXPLICIT;
}

SetFormulaBasic *SetFormulaExplicit::get_constant_formula(SetFormulaConstant *c_formula) {
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

bool SetFormulaExplicit::contains(const Cube &statecube) {
    BDD tmp = util->build_bdd_from_cube(statecube);
    return tmp.Leq(set);
}

