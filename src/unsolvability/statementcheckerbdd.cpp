#include "statementcheckerbdd.h"

StatementCheckerBDD::StatementCheckerBDD(KnowledgeBase *kb, Task *task, const std::vector<int> &variable_permutation)
    : StatementChecker(kb, task), variable_permutation(variable_permutation) {

    assert(this->variable_permutation.size() == task->get_number_of_facts());

    prime_permutation.resize(task->get_number_of_facts()*2, -1);
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      prime_permutation[2*i] = (2*i)+1;
      prime_permutation[(2*i)+1] = 2*i;
    }

    bdds.insert("S_I", build_bdd_from_cube(task->get_initial_state()));
    bdds.insert("S_G", build_bdd_from_cube(task->get_goal()));
    // the BDD representing the empty set should still be indifferent about primed variables
    Cube empty_cube(task->get_number_of_facts()*2,2);
    for(int i = 0; i < task->get_number_of_facts(); ++i) {
        empty_cube[2*i] = 0;
    }
    bdds.insert("empty",build_bdd_from_cube(empty_cube));

    initial_state_bdd = &(bdds.find("S_I")->second);
    goal_bdd = &(bdds.find("S_G")->second);
    empty_bdd = &(bdds.find("empty")->second);
}

BDD StatementCheckerBDD::build_bdd_from_cube(const Cube &cube) {
    assert(cube.size() == task->get_number_of_facts());
    std::vector<int> local_cube(cube.size()*2,2);
    for(size_t i = 0; i < cube.size(); ++i) {
        //permute both accounting for primed variables and changed var order
        local_cube[variable_permutation[i]*2] = cube[i];
    }
    return BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &local_cube[0]));
}

BDD StatementCheckerBDD::build_bdd_for_action(const Action &a) {
    BDD ret = manager.bddOne();
    for(size_t i = 0; i < a.pre.size(); ++i) {
        ret = ret * manager.bddVar(a.pre[i]*2);
    }

    //+1 represents primed variable
    for(size_t i = 0; i < a.change.size(); ++i) {
        int permutated_i = variable_permutation[i];
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

bool StatementCheckerBDD::check_initial_contained(std::string set) {
    assert(bdds.find(set) != bdds.end());
    BDD &set_bdd = bdds.find(set)->second;
    if(!(*initial_state_bdd).Leq(set_bdd)) {
        return false;
    } else {
        kb->insert_contains_initial(set);
        return true;
    }
}

bool StatementCheckerBDD::check_is_contained(Cube state, std::string set) {
    assert(bdds.find(set) != bdds.end());
    BDD &set_bdd = bdds.find(set)->second;
    BDD state_bdd = build_bdd_from_cube(state);
    if(!state_bdd.Leq(set_bdd)) {
        return false;
    } else {
        kb->insert_contained_in(state,set);
        return true;
    }
}

bool StatementCheckerBDD::check_progression(std::string set1, std::string set2) {
    assert(bdds.find(set1) != bdds.end() && bdds.find(set2) != bdds.end());
    BDD &set1_bdd = bdds.find(set1)->second;
    BDD &set2_bdd = bdds.find(set2)->second;

    BDD possible_successors = (set1_bdd + set2_bdd).Permute(&prime_permutation[0]);

    // loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);
        // succ represents pairs of states and its successors achieved with action a
        BDD succ = action_bdd * set1_bdd;

        // test if succ is a subset of all "allowed" successors
        if(!succ.Leq(possible_successors)) {
            return false;
        }
    }
    kb->insert_progression(set1,set2);
    return true;
}

bool StatementCheckerBDD::check_regression(std::string set1, std::string set2) {
    assert(bdds.find(set1) != bdds.end() && bdds.find(set2) != bdds.end());
    BDD &set1_bdd = bdds.find(set1)->second;
    BDD &set2_bdd = bdds.find(set2)->second;

    BDD possible_predecessors = set1_bdd + set2_bdd;

    // loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);
        // pred represents pairs of states and its predecessors from action a
        BDD pred = action_bdd * set1_bdd.Permute(&prime_permutation[0]);

        // test if pred is a subset of all "allowed" predecessors
        if(!pred.Leq(possible_predecessors)) {
            return false;
        }
    }
    kb->insert_regression(set1,set2);
    return true;
}

bool StatementCheckerBDD::check_subset(std::string set1, std::string set2) {
    assert(bdds.find(set1) != bdds.end() && bdds.find(set2) != bdds.end());
    if(!bdds.find(set1)->second.Leq(bdds.find(set2)->second)) {
        return false;
    } else {
        kb->insert_subset(set1,set2);
        return true;
    }
}

void StatementCheckerBDD::read_in_sets(std::string filename) {

}
