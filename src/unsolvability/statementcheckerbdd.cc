#include "statementcheckerbdd.h"
#include "global_funcs.h"

#include "dddmp.h"

#include <stack>
#include <cassert>
#include <sstream>
#include <iostream>

StatementCheckerBDD::StatementCheckerBDD(KnowledgeBase *kb, Task *task, std::ifstream &in)
    : StatementChecker(kb, task), manager(Cudd(task->get_number_of_facts()*2,0)) {


    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);

    std::string line;

    // first line contains variable permutation
    std::getline(in, line);
    std::istringstream iss(line);
    int n;
    while (iss >> n){
        variable_permutation.push_back(n);
    }
    assert(this->variable_permutation.size() == task->get_number_of_facts());

    // used for changing bdd to primed variables
    prime_permutation.resize(task->get_number_of_facts()*2, -1);
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      prime_permutation[2*i] = (2*i)+1;
      prime_permutation[(2*i)+1] = 2*i;
    }

    // insert BDDs for initial state, goal and empty set
    bdds.insert(std::make_pair(special_set_string(SpecialSet::INITSET), build_bdd_from_cube(task->get_initial_state())));
    bdds.insert(std::make_pair(special_set_string(SpecialSet::GOALSET), build_bdd_from_cube(task->get_goal())));
    bdds.insert(std::make_pair(special_set_string(SpecialSet::EMPTYSET), manager.bddZero()));

    initial_state_bdd = &(bdds.find(special_set_string(SpecialSet::INITSET))->second);
    goal_bdd = &(bdds.find(special_set_string(SpecialSet::GOALSET))->second);
    empty_bdd = &(bdds.find(special_set_string(SpecialSet::EMPTYSET))->second);

    // second line contains bdd names separated by semicolons
    std::vector<std::string> bdd_names;
    std::getline(in,line);
    std::stringstream ss(line);
    while(std::getline(ss,line,';')) {
        bdd_names.push_back(line);
    }
    // third line contains file name for bdds
    std::getline(in, line);
    read_in_bdds(line,bdd_names);

    std::getline(in, line);
    //read in composite formulas
    if(line.compare("composite formulas begin") == 0) {
        read_in_composite_formulas(in);
        std::getline(in, line);
    }
    // second last line contains location of statement file
    statementfile = line;

    // last line declares end of Statement block
    std::getline(in, line);
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
        ret = ret * manager.bddVar(variable_permutation[a.pre[i]]*2);
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

void StatementCheckerBDD::read_in_bdds(std::string filename, std::vector<std::string> &bdd_names) {

    // move variables so the primed versions are in between
    int compose[task->get_number_of_facts()];
    for(int i = 0; i < task->get_number_of_facts(); ++i) {
        compose[i] = 2*i;
    }

    FILE *fp;
    fp = fopen(filename.c_str(), "r");
    if(!fp) {
        std::cerr << "could not open bdd file" << std::endl;
    }

    DdNode **tmpArray;
    int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
        DDDMP_VAR_COMPOSEIDS,NULL,NULL,&compose[0],DDDMP_MODE_TEXT,NULL,fp,&tmpArray);
    assert(nRoots == bdd_names.size());

    for (int i=0; i<nRoots; i++) {
        // TODO: check how names could be saved in the bdd file and replace dummy names
        bdds.insert(std::make_pair(bdd_names[i], BDD(manager,tmpArray[i])));
        Cudd_RecursiveDeref(manager.getManager(), tmpArray[i]);
    }
    FREE(tmpArray);
}

// composite formulas are denoted in POSTFIX notation
void StatementCheckerBDD::read_in_composite_formulas(std::ifstream &in) {
    std::string line;
    std::getline(in, line);
    std::vector<BDD> tmp_bdds;
    while(line.compare("composite formulas end") != 0) {
        bdds.insert(std::make_pair(line, get_BDD(line, tmp_bdds)));
        std::getline(in, line);
    }
}

// returns a reference to a BDD that is either already saved in the bdds map or temporarily
// constructed in the tmp_bdds vector
// various tmp BDDs needed in the construction may get added to the tmp_bdds vector
BDD &StatementCheckerBDD::get_BDD(const std::string &description, std::vector<BDD> &tmp_bdds) {
    auto found = bdds.find(description);
    if(found != bdds.end()) {
        return found->second;
    }

    int tmp_old_size = tmp_bdds.size();
    std::stack<BDD *> elements;
    std::stringstream ss;
    ss.str(description);
    std::string item;
    while(std::getline(ss, item, ' ')) {
        if(item.compare(KnowledgeBase::INTERSECTION) == 0) {
            assert(elements.size() >=2);
            BDD *right = elements.top();
            elements.pop();
            BDD *left = elements.top();
            elements.pop();
            tmp_bdds.push_back((*left)*(*right));
            elements.push(&(tmp_bdds.at(tmp_bdds.size()-1)));
        } else if(item.compare(KnowledgeBase::UNION) == 0) {
            assert(elements.size() >=2);
            BDD *right = elements.top();
            elements.pop();
            BDD *left = elements.top();
            elements.pop();
            tmp_bdds.push_back((*left)+(*right));
            elements.push(&(tmp_bdds.at(tmp_bdds.size()-1)));
        } else if(item.compare(KnowledgeBase::NEGATION) == 0) {
            assert(elements.size() >= 1);
            BDD *elem = elements.top();
            elements.pop();
            tmp_bdds.push_back(!(*elem));
            elements.push(&(tmp_bdds.at(tmp_bdds.size()-1)));
        } else {
            assert(bdds.find(item) != bdds.end());
            elements.push(&(bdds.find(item)->second));
        }
    }
    assert(elements.size() == 1 && tmp_bdds.size() > tmp_old_size);
    return tmp_bdds[tmp_bdds.size()-1];
}

bool StatementCheckerBDD::check_subset(const std::string &set1, const std::string &set2) {
    std::vector<BDD> tmp_bdds;
    BDD &bdd1 = get_BDD(set1, tmp_bdds);
    BDD &bdd2 = get_BDD(set2, tmp_bdds);
    if(bdd1.Leq(bdd2)) {
        return true;
    }
    return false;
}

bool StatementCheckerBDD::check_progression(const std::string &set1, const std::string &set2) {
    std::vector<BDD> tmp_bdds;
    BDD &bdd1 = get_BDD(set1, tmp_bdds);
    BDD &bdd2 = get_BDD(set2, tmp_bdds);

    BDD possible_successors = (bdd1 + bdd2).Permute(&prime_permutation[0]);

    // loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);
        // succ represents pairs of states and its successors achieved with action a
        BDD succ = action_bdd * bdd1;

        // test if succ is a subset of all "allowed" successors
        if(!succ.Leq(possible_successors)) {
            return false;
        }
    }
    return true;
}

bool StatementCheckerBDD::check_regression(const std::string &set1, const std::string &set2) {
    std::vector<BDD> tmp_bdds;
    BDD &bdd1 = get_BDD(set1, tmp_bdds);
    BDD &bdd2 = get_BDD(set2, tmp_bdds);

    BDD possible_predecessors = bdd1 + bdd2;

    // loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);
        // pred represents pairs of states and its predecessors from action a
        BDD pred = action_bdd * bdd1.Permute(&prime_permutation[0]);

        // test if pred is a subset of all "allowed" predecessors
        if(!pred.Leq(possible_predecessors)) {
            return false;
        }
    }
    return true;
}

bool StatementCheckerBDD::check_is_contained(Cube &state, const std::string &set) {
    std::vector<BDD> tmp_bdds;
    BDD &set_bdd = get_BDD(set, tmp_bdds);
    BDD state_bdd = build_bdd_from_cube(state);
    if(!state_bdd.Leq(set_bdd)) {
        return false;
    }
    return true;
}

bool StatementCheckerBDD::check_initial_contained(const std::string &set) {
    std::vector<BDD> tmp_bdds;
    BDD &set_bdd = get_BDD(set, tmp_bdds);
    if(!(*initial_state_bdd).Leq(set_bdd)) {
        return false;
    }
    return true;
}

// TODO: check if variable permutation is applied correctly
bool StatementCheckerBDD::check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) {
    std::vector<BDD> tmp_bdds;
    BDD &set_bdd = get_BDD(set, tmp_bdds);
    if(set_bdd.IsZero()) {
        return true;
    // if the BDD contains more models than the stateset, it cannot be a subset
    // TODO: the computation is bugged
    /*} *else if(set_bdd.CountMinterm(task->get_number_of_facts()*2)/(1<<task->get_number_of_facts()) > stateset.getSize()) {
        return false;*/
    }

    int* bdd_model;
    Cube statecube(task->get_number_of_facts(),-1);
    CUDD_VALUE_TYPE value_type;
    DdManager *ddmgr = manager.getManager();
    DdNode *ddnode = set_bdd.getNode();
    DdGen * cubegen = Cudd_FirstCube(ddmgr,ddnode,&bdd_model, &value_type);
    // the models gotten with FirstCube and NextCube can contain don't cares, but this doesn't matter since stateset.contains() can deal with this
    do{
        for(int i = 0; i < statecube.size(); ++i) {
            statecube[i] = bdd_model[2*variable_permutation[i]];
        }
        if(!stateset.contains(statecube)) {
            std::cout << "stateset does not contain the following cube:";
            for(int i = 0; i < statecube.size(); ++i) {
                std::cout << statecube[i] << " ";
            } std::cout << std::endl;
            return false;
        }
    } while(Cudd_NextCube(cubegen,&bdd_model,&value_type) != 0);
    return true;
}
