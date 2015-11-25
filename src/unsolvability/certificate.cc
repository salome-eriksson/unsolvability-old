#include "certificate.h"
#include <string.h>
#include <cassert>
#include <fstream>
#include <stdlib.h>
#include <algorithm>

#include "dddmp.h"

Certificate::Certificate(Task *task)
    : task(task), manager(Cudd(task->get_number_of_facts()*2,0)) {

}

Certificate::~Certificate() {

}

/*
 * In the bdd representation primed facts immediately follow their unprimed version
 * Facts that do not occur in the bdd will be mapped to variables with highest index
 * fact_to_bddvar[i] = j means that the variable with global index i has index j in the bdd
 */
void Certificate::read_in_variable_order(std::ifstream &in) {
    int fact_amount = task->get_number_of_facts();
    fact_to_bddvar = std::vector<int>(fact_amount, -1);
    int count = 0;
    std::string fact;
    std::getline(in, fact);
    while(fact.compare("end_variables") != 0) {
        int global_index = task->find_fact(fact);
        // *2 in order to account for primed variables
        fact_to_bddvar[global_index] = 2*count;
        count++;
        if(!std::getline(in, fact)) {
            std::cout << "file ended without \"end_variables\"" << std::endl;
            exit(0);
        }
    }
    original_bdd_vars = count;

    // fill in mapping for facts that don't occur in the bdd
    for(size_t i = 0; i < fact_amount; ++i) {
        if(fact_to_bddvar[i] == -1) {
            fact_to_bddvar[i] = 2*count;
            count++;
        }
    }
    assert(count == fact_amount);
    // make sure that all global facts map to a valid bbd index
    assert(std::find(fact_to_bddvar.begin(), fact_to_bddvar.end(), -1) == fact_to_bddvar.end());
}

void Certificate::parse_bdd_file(std::string bddfile, std::vector<BDD> &bdds) {
    assert(bdds.empty());

    // move variables so the primed versions are in between
    int compose[original_bdd_vars];
    for(int i = 0; i < original_bdd_vars; ++i) {
        compose[i] = 2*i;
    }

    DdNode **tmpArray;
    int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
        DDDMP_VAR_COMPOSEIDS,NULL,NULL,&compose[0],DDDMP_MODE_TEXT,&bddfile[0],NULL,&tmpArray);

    bdds.resize(nRoots);

    for (int i=0; i<nRoots; i++) {
        bdds[i] = BDD(manager, tmpArray[i]);
        Cudd_RecursiveDeref(manager.getManager(), tmpArray[i]);
    }
    FREE(tmpArray);
}

//TODO: maybe change to IndicesToCube?
//TODO: move?
BDD Certificate::build_bdd_for_action(const Action &a) {
    BDD ret = manager.bddOne();
    for(size_t i = 0; i < a.pre.size(); ++i) {
        ret = ret * manager.bddVar(fact_to_bddvar[a.pre[i]]);
    }

    //+1 represents primed variable
    for(size_t i = 0; i < a.change.size(); ++i) {
        int local_var = fact_to_bddvar[i];
        //add effect
        if(a.change[i] == 1) {
            ret = ret * manager.bddVar(local_var+1);
        //delete effect
        } else if(a.change[i] == -1) {
            ret = ret - manager.bddVar(local_var+1);
        //no change -> frame axiom
        } else {
            assert(a.change[i] == 0);
            ret = ret * (manager.bddVar(local_var) + !manager.bddVar(local_var+1));
            ret = ret * (!manager.bddVar(local_var) + manager.bddVar(local_var+1));
        }
    }
    return ret;
}


bool Certificate::is_certificate_for(const Cube &s) {
    print_info("checking if certificate contains state");
    bool has_s = contains_cube(s);
    print_info("checking if certificate contains goal");
    bool has_goal = contains_cube(task->get_goal());
    print_info("checking if certificate is inductive");
    bool inductivity = is_inductive();
    bool valid = has_s && !has_goal && inductivity;

    if(!valid) {
        std::cout << "The certificate is NOT valid because: " << std::endl;
        if(!has_s) {
            std::cout << " - it does NOT contain the initial state" << std::endl;
        }
        if(has_goal) {
            std::cout << " - it contains the goal state" << std::endl;
        }
        if(!inductivity) {
            std::cout << " - it is NOT inductive" << std::endl;
        }
        return false;
    }
    return true;
}


void Certificate::dump_bdd(BDD &bdd, std::string filename) {
    int n = task->get_number_of_facts();
    std::vector<std::string> names(n*2);
    for(size_t i = 0; i < n; ++i) {
        names[fact_to_bddvar[i]] = task->get_fact(i);
        names[fact_to_bddvar[i]+1] = task->get_fact(i) + "'";
    }

    char **nameschar = new char*[names.size()];
    for(size_t i = 0; i < names.size(); i++){
        nameschar[i] = new char[names[i].size() + 1];
        strcpy(nameschar[i], names[i].c_str());
    }
    std::string bddname = "bla";
    FILE* f = fopen(filename.c_str(), "w");
    Dddmp_cuddBddStore(manager.getManager(),&bddname[0], bdd.getNode(), nameschar, NULL,
            DDDMP_MODE_TEXT, DDDMP_VARNAMES, &filename[0], f);
    fclose(f);
}
