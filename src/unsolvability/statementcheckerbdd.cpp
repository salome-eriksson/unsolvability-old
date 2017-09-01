#include "statementcheckerbdd.h"

#include <stack>

StatementCheckerBDD::StatementCheckerBDD(KnowledgeBase *kb, Task *task, std::ifstream &in)
    : StatementChecker(kb, task), manager(Cudd(task->get_number_of_facts()*2,0)) {

    std::string line;

    //first line contains variable permutation
    std::getline(in, line);
    std::istringstream iss(line);
    int n;
    while (iss >> n){
        variable_permutation.push_back(n);
    }
    assert(this->variable_permutation.size() == task->get_number_of_facts());

    //used for changing bdd to primed variables
    prime_permutation.resize(task->get_number_of_facts()*2, -1);
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      prime_permutation[2*i] = (2*i)+1;
      prime_permutation[(2*i)+1] = 2*i;
    }

    //insert BDDs for initial state, goal and empty set
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

    //second line contains file name for bdds
    std::getline(in, line);
    read_in_bdds(line);

    std::getline(in, line);
    if(line.compare("composite formulas begin") == 0) {
        read_in_composite_formulas(in);
        std::getline(in, line);
    }
    if(line.compare("statements begin") == 0) {
        read_in_statements(in);
        std::getline(in, line);
    }
    assert(line.compare("Statements:BDD end") == 0);
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

void StatementCheckerBDD::read_in_bdds(std::string filename) {

}

// composite formulas are denoted in POSTFIX notation
void StatementCheckerBDD::read_in_composite_formulas(std::ifstream &in) {
    std::string line;
    std::getline(in, line);

    std::stack<std::pair<std::string,BDD>> elements;
    while(line.compare("composite formulas end") != 0) {
        std::stringstream ss;
        ss.str(line);
        std::string item;
        while(std::getline(ss, item, " ")) {
            switch(item) {
            case KnowledgeBase::INTERSECTION:
                assert(elements.size() >=2);
                std::pair<std::string,BDD> left = elements.top();
                elements.pop();
                std::pair<std::string,BDD> right = elements.top();
                elements.pop();
                elements.push(make_pair(left.first + " " + right.first + " " + KnowledgeBase::INTERSECTION, left.second * right.second));
                break;
            case KnowledgeBase::UNION:
                assert(elements.size() >=2);
                std::pair<std::string,BDD> left = elements.top();
                elements.pop();
                std::pair<std::string,BDD> right = elements.top();
                elements.pop();
                elements.push(make_pair(left.first + " " + right.first + " " + KnowledgeBase::UNION, left.second + right.second));
                break;
            case KnowledgeBase::NEGATION:
                assert(elements.size() >= 1);
                std::pair<std::string,BDD> elem = elements.top();
                elements.pop();
                elements.push(make_pair(elem.first + " " + KnowledgeBase::NEGATION, !elem.second));
                break;
            default:
                assert(bdds.find(item) != bdds.end);
                elements.push(make_pair(item,bdds.find(item)->second));
                break;
            }
        }
        assert(elements.size() == 1);
        std::pair<std::string,BDD> result = elements.top;
        bdds.insert(result.first, result.second);
        std::getline(in, line);
    }
}

void StatementCheckerBDD::read_in_statements(std::ifstream &in) {
    std::string line;
    std::getline(in, line);
    while(line.compare("statements end") != 0) {
        int pos_colon = s.find(":");
        Statement statement = Statement(line.substr(0, pos_colon));
        switch(statement) {
        case Statement::SUBSET:
            break;
        case Statement::EXPLICIT_SUBSET:
            break;
        case Statement::PROGRESSION:
            break;
        case Statement::REGRESSION:
            break;
        case Statement::CONTAINED:
            break;
        case Statement::INITIAL_CONTAINED:
            break;
        default:
            std::err << "unkown statement: " << statement;
            break;
        }
    }
}


bool StatementCheckerBDD::check_initial_contained(const std::string &set) {
    assert(bdds.find(set) != bdds.end());
    BDD &set_bdd = bdds.find(set)->second;
    if(!(*initial_state_bdd).Leq(set_bdd)) {
        return false;
    } else {
        kb->insert_contains_initial(set);
        return true;
    }
}

bool StatementCheckerBDD::check_is_contained(const Cube &state, const std::string &set) {
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

bool StatementCheckerBDD::check_progression(const std::string &set1, const std::string &set2) {
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

bool StatementCheckerBDD::check_regression(const std::string &set1, const std::string &set2) {
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

bool StatementCheckerBDD::check_subset(const std::string &set1, const std::string &set2) {
    assert(bdds.find(set1) != bdds.end() && bdds.find(set2) != bdds.end());
    if(!bdds.find(set1)->second.Leq(bdds.find(set2)->second)) {
        return false;
    } else {
        kb->insert_subset(set1,set2);
        return true;
    }
}

bool StatementCheckerBDD::check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) {
    assert(bdds.find(set) != bdds.end());
    BDD &set_bdd = bdds.find(set)->second;
    if(set_bdd.isZero()) {
        return true;
    }
    std::vector<int> statecube(task->get_number_of_facts(),0);
    int model[task->get_number_of_facts()*2];
    DdGen *iterator = set_bdd.FirstCube(model);
    do {
        for(int i = 0; i < statecube.size(); ++i) {
            statecube[i] = model[2*i];
        }
        if(!stateset.contains(statecube)) {
            return false;
        }
    } while(NextCube(iterator,model) != 0);
    kb->insert_subset(set, stateset.getName());
    return true;
}
