#include "certificate.h"
#include <string.h>
#include <cassert>
#include <fstream>
#include <stdlib.h>

#include "dddmp.h"

Certificate::Certificate(Task *task) : task(task) {

}

Certificate::~Certificate() {

}

/*
 * In the bdd representation primed facts immediately follow their unprimed version
 * Facts that do not occur in the bdd will be mapped to variables with highest index
 * fact_to_bddvar[i] = j means that the variable with global index i has index j in the bdd
 */
void Certificate::map_global_facts_to_bdd_var(std::ifstream &in) {
    int fact_amount = task->get_number_of_facts();
    fact_to_bddvar = std::vector<int>(fact_amount, -1);
    int count = 0;
    std::string fact;
    std::getline(in, fact);
    while(fact.compare("end_variables") != 0) {
        int global_index = task->find_fact(fact);
        fact_to_bddvar[global_index] = 2*count;
        count++;
        if(!std::getline(in, fact)) {
            std::cout << "file ended without \"end_variables\"" << std::endl;
            exit(0);
        }
    }
    original_bdd_vars = count;

    //fill in mapping for facts that don't occur in the bdd
    for(size_t i = 0; i < fact_amount; ++i) {
        if(fact_to_bddvar[i] == -1) {
            fact_to_bddvar[i] = 2*count;
            count++;
        }
    }
    assert(count == fact_amount);
}

void Certificate::parse_bdd_file(std::string bddfile, std::vector<BDD> &bdds) {
    //move variables so the primed versions are in between
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
        Cudd_RecursiveDeref (manager.getManager(), tmpArray[i]);
    }
    FREE(tmpArray);
}

BDD Certificate::build_bdd_for_action(const Action &a) {
    BDD ret = manager.bddOne();
    for(size_t i = 0; i < a.pre.size(); ++i) {
        ret = ret * manager.bddVar(fact_to_bddvar[a.pre[i]]);
    }
    //the +1 below is for getting the primed version of the variable
    for(size_t i = 0; i < a.add.size(); ++i) {
        ret = ret * manager.bddVar(fact_to_bddvar[a.add[i]]+1);
    }
    for(size_t i = 0; i < a.del.size(); ++i) {
        ret = ret - manager.bddVar(fact_to_bddvar[a.del[i]]+1);
    }
    return ret;
}
