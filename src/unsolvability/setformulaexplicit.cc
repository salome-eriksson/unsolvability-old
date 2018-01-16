#include "setformulaexplicit.h"
#include "setformulahorn.h"
#include "setformulabdd.h"

#include <cassert>
#include <fstream>

#include "global_funcs.h"


ExplicitUtil::ExplicitUtil(Task *task)
    : manager(Cudd(task->get_number_of_facts()*2,0)) {


    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();

    // move variables so the primed versions are in between
    int compose[task->get_number_of_facts()];
    for(int i = 0; i < task->get_number_of_facts(); ++i) {
        compose[i] = 2*i;
    }

    // set up special formulas
    actionformulas.reserve(task->get_number_of_actions());
    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        actionformulas.push_back(build_bdd_for_action(task->get_action(i)));
    }

    // insert BDDs for initial state, goal and empty set
    initformula = new SetFormulaExplicit(build_bdd_from_cube(task->get_initial_state()));
    goalformula = new SetFormulaExplicit(build_bdd_from_cube(task->get_goal()));
    emptyformula = new SetFormulaExplicit(BDD(manager.bddZero()));

    // this shuffels a BDD to represent the primed variables
    prime_permutation.resize(task->get_number_of_facts()*2, -1);
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      prime_permutation[2*i] = (2*i)+1;
      prime_permutation[(2*i)+1] = 2*i;
    }
}

BDD ExplicitUtil::build_bdd_from_cube(const Cube &cube) {
    std::vector<int> local_cube(cube.size()*2,2);
    for(size_t i = 0; i < cube.size(); ++i) {
        //permute both accounting for primed variables
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

ExplicitUtil *SetFormulaExplicit::util = nullptr;

SetFormulaExplicit::SetFormulaExplicit(BDD set)
    : set(set) {

}

SetFormulaExplicit::SetFormulaExplicit(std::ifstream &input, Task *task) {
    if(!util) {
        util = new ExplicitUtil(task);
    }
    set = util->manager.bddZero();
    std::string s;
    input >> s;
    while(s.compare(";") != 0) {
        set += util->build_bdd_from_cube(parseCube(s, task->get_number_of_facts()));
        input >> s;
    }
}


bool SetFormulaExplicit::is_subset(SetFormula *f, bool negated, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    }
    switch (f->get_formula_type()) {
    case SetFormulaType::HORN: {
        if(negated || f_negated) {
            std::cerr << "L \\subseteq L' not supported for Explicit formula L and ";
            std::cerr << "Horn formula L' if one of them is negated" << std::endl;
            return false;
        }

        const SetFormulaHorn *f_horn = static_cast<SetFormulaHorn *>(f);
        for(int i : f_horn->get_forced_false()) {
            BDD tmp = !(util->manager.bddVar(i*2));
            if(!(set.Leq(tmp))) {
                return false;
            }
        }

        for(int i = 0; i < f_horn->get_forced_true().size(); ++i) {
            BDD tmp = (util->manager.bddVar(i*2));
            if(!(set.Leq(tmp))) {
                return false;
            }
        }

        for(int i = 0; i < f_horn->get_size(); ++i) {
            BDD tmp = util->manager.bddZero();
            for(int j : f_horn->get_left_vars(i)) {
                tmp += !(util->manager.bddVar(j*2));
            }
            if(f_horn->get_right(i) != -1) {
                tmp += (util->manager.bddVar(i*2));
            }
            if(!(set.Leq(tmp))) {
                return false;
            }
        }
        return true;
        break;
    }
    case SetFormulaType::BDD: {
        if(negated || f_negated) {
            std::cerr << "L \\subseteq L' not supported for Explicit formula L and ";
            std::cerr << "BDD formula L' if one of them is negated" << std::endl;
            return false;
        }

        SetFormulaBDD *f_bdd = static_cast<SetFormulaBDD *>(f);

        int* bdd_model;
        // TODO: find a better way to find out how many variables we have
        Cube statecube(util->prime_permutation.size()/2,-1);
        CUDD_VALUE_TYPE value_type;
        DdManager *ddmgr = util->manager.getManager();
        DdNode *ddnode = set.getNode();
        DdGen * cubegen = Cudd_FirstCube(ddmgr,ddnode,&bdd_model, &value_type);
        /* the models gotten with FirstCube and NextCube can contain don't cares,
         * but SetFormulaBDD::contains can handle this
         */
        do{
            for(int i = 0; i < statecube.size(); ++i) {
                statecube[i] = bdd_model[2*i];
            }
            if(!f_bdd->contains(statecube)) {
                std::cout << "SetFormulaBDD does not contain the following cube:";
                for(int i = 0; i < statecube.size(); ++i) {
                    std::cout << statecube[i] << " ";
                } std::cout << std::endl;
                return false;
            }
        } while(Cudd_NextCube(cubegen,&bdd_model,&value_type) != 0);
        return true;
        break;
    }
    case SetFormulaType::TWOCNF:
        std::cerr << "not implemented yet";
        return false;
        break;
    case SetFormulaType::EXPLICIT: {
        const SetFormulaExplicit *f_expl = static_cast<SetFormulaExplicit *>(f);
        BDD left = set;
        if(negated) {
            left = !left;
        }
        BDD right = f_expl->set;
        if(f_negated) {
            right = !right;
        }
        return left.Leq(right);
        break;
    }
    default:
        std::cerr << "X \\subseteq X' is not supported for explicit formula X "
                     "and non-basic or constant formula X'" << std::endl;
        return false;
        break;
    }
}


bool SetFormulaExplicit::is_subset(SetFormula *f1, SetFormula *f2) {
    if(f1->get_formula_type() == SetFormulaType::CONSTANT) {
        f1 = get_constant_formula(static_cast<SetFormulaConstant *>(f1));
    }
    if(f2->get_formula_type() == SetFormulaType::CONSTANT) {
        f2 = get_constant_formula(static_cast<SetFormulaConstant *>(f2));
    }
    if(f1->get_formula_type() != SetFormulaType::EXPLICIT ||
            f2->get_formula_type() != SetFormulaType::EXPLICIT) {
        std::cerr << "X \\subseteq X' \\cup X'' is not supported for explicit formula X ";
        std::cerr << "and non-explicit formula X' or X''" << std::endl;
        return false;
    }
    const SetFormulaExplicit *f1_expl = static_cast<SetFormulaExplicit *>(f1);
    const SetFormulaExplicit *f2_expl = static_cast<SetFormulaExplicit *>(f2);
    return set.Leq(f1_expl->set + f2_expl->set);
}


bool SetFormulaExplicit::intersection_with_goal_is_subset(SetFormula *f, bool negated, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    } else if(f->get_formula_type() != SetFormulaType::EXPLICIT) {
        std::cerr << "L \\cap S_G(\\Pi) \\subseteq L' is not supported for explicit Formula L ";
        std::cerr << "and non-explicit formula L'" << std::endl;
        return false;
    }
    const SetFormulaExplicit *f_expl = static_cast<SetFormulaExplicit *>(f);
    BDD left = set;
    if(negated) {
        left = !left;
    }
    left = left * util->goalformula->set;
    BDD right = f_expl->set;
    if(f_negated) {
        right = !right;
    }
    return left.Leq(right);
}


bool SetFormulaExplicit::progression_is_union_subset(SetFormula *f, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    } else if(f->get_formula_type() != SetFormulaType::EXPLICIT) {
        std::cerr << "X[a] \\subseteq X \\land L is not supported for explicit Formula X ";
        std::cerr << "and non-explicit formula L" << std::endl;
        return false;
    }
    const SetFormulaExplicit *f_expl = static_cast<SetFormulaExplicit *>(f);
    BDD possible_successors = f_expl->set;
    if(f_negated) {
        possible_successors = !possible_successors;
    }
    possible_successors += set;
    possible_successors = possible_successors.Permute(&util->prime_permutation[0]);

    for(int i = 0; i < util->actionformulas.size(); ++i) {
        BDD succ = set * util->actionformulas[i];
        if(!succ.Leq(possible_successors)) {
            return false;
        }
    }
    return true;
}


bool SetFormulaExplicit::regression_is_union_subset(SetFormula *f, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(static_cast<SetFormulaConstant *>(f));
    } else if(f->get_formula_type() != SetFormulaType::EXPLICIT) {
        std::cerr << "X[a] \\subseteq X \\land L is not supported for explicit Formula X ";
        std::cerr << "and non-explicit formula L" << std::endl;
        return false;
    }
    const SetFormulaExplicit *f_expl = static_cast<SetFormulaExplicit *>(f);
    BDD possible_predecessors = f_expl->set;
    if(f_negated) {
        possible_predecessors = !possible_predecessors;
    }
    possible_predecessors += set;

    for(int i = 0; i < util->actionformulas.size(); ++i) {
        BDD pred = set.Permute(&util->prime_permutation[0]) * util->actionformulas[i];
        if(!pred.Leq(possible_predecessors)) {
            return false;
        }
    }
    return true;
}


SetFormulaType SetFormulaExplicit::get_formula_type() {
    return SetFormulaType::EXPLICIT;
}

SetFormulaBasic *SetFormulaExplicit::get_constant_formula(SetFormulaConstant *c_formula) {
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

bool SetFormulaExplicit::contains(const Cube &statecube) {
    BDD tmp = util->build_bdd_from_cube(statecube);
    return tmp.Leq(set);
}

