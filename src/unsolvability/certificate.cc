#include "certificate.h"

#include <string.h>
#include <cassert>
#include <fstream>
#include <stdlib.h>
#include <algorithm>

#include "dddmp.h"

Certificate::Certificate(Task *task)
    : task(task), manager(Cudd(task->get_number_of_facts()*2,0)) {
    if(g_timeout > 0) {
        manager.SetTimeLimit(g_timeout*1000 - timer());
    }
    manager.setTimeoutHandler(exit_timeout);
}

Certificate::~Certificate() {

}


void Certificate::parse_bdd_file(std::string bddfile) {
    print_info("reading in bdd file " + bddfile);

    // move variables so the primed versions are in between
    int compose[task->get_number_of_facts()];
    for(int i = 0; i < task->get_number_of_facts(); ++i) {
        compose[i] = 2*i;
    }

    FILE *fp;
    fp = fopen(bddfile.c_str(), "r");
    if(!fp) {
        std::cout << "could not open bdd file" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    int amount = -1;
    while(fscanf(fp, "%d", &amount) == 1) {
        assert(amount > 0);
        std::vector<int> indices = std::vector<int>(amount, -1);
        for(int i = 0; i < amount; ++i) {
            int res = fscanf(fp, "%d", &indices[i]);
            assert(res == 1);
            assert(indices[i] >= 0 && certificate.find(indices[i]) == certificate.end());
        }
        DdNode **tmpArray;
        int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
            DDDMP_VAR_COMPOSEIDS,NULL,NULL,&compose[0],DDDMP_MODE_TEXT,NULL,fp,&tmpArray);
        assert(nRoots == amount);

        for (int i=0; i<nRoots; i++) {
            certificate[indices[i]] = CertEntry(BDD(manager, tmpArray[i]), false);
            Cudd_RecursiveDeref(manager.getManager(), tmpArray[i]);
        }
        FREE(tmpArray);
        amount = -1;
    }

    print_info("finished reading in bdd file " + bddfile);
}

//TODO: maybe change to IndicesToCube? But how to deal with frame axioms?
//TODO: move?
BDD Certificate::build_bdd_for_action(const Action &a) {
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

//TODO: move?
BDD Certificate::build_bdd_from_cube(const Cube &cube) {
    assert(cube.size() == task->get_number_of_facts());
    std::vector<int> local_cube(cube.size()*2,2);
    for(size_t i = 0; i < cube.size(); ++i) {
        local_cube[i*2] = cube[i];
    }
    return BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &local_cube[0]));
}


bool Certificate::is_certificate_for(const Cube &s) {
    print_info("checking if certificate contains state");
    bool has_s = contains_state(s);
    print_info("checking if certificate contains goal");
    bool has_goal = contains_goal();
    print_info("checking if certificate is inductive");
    bool inductivity = is_inductive();
    bool valid = has_s && !has_goal && inductivity;

    if(!valid) {
        print_info("The certificate is NOT valid because:");
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
        names[2*i] = task->get_fact(i);
        names[2*i+1] = task->get_fact(i) + "'";
    }

    char **nameschar = new char*[names.size()];
    for(size_t i = 0; i < names.size(); i++){
        nameschar[i] = new char[names[i].size() + 1];
        strcpy(nameschar[i], names[i].c_str());
    }
    std::string bddname = filename;
    FILE* f = fopen(filename.c_str(), "w");
    Dddmp_cuddBddStore(manager.getManager(),&bddname[0], bdd.getNode(), nameschar, NULL,
            DDDMP_MODE_TEXT, DDDMP_VARNAMES, &filename[0], f);
    fclose(f);
}
