#include "cudd_interface.h"
#include "../globals.h"

#ifdef USE_CUDD
#include "dddmp.h"

std::vector<std::vector<int> > BDDWrapper::fact_ids = std::vector<std::vector<int> >();
Cudd* BDDWrapper::manager = NULL;
char** BDDWrapper::fact_names = NULL;


BDDWrapper::BDDWrapper() {
    if(manager == NULL) {
        BDDWrapper::initializeManager();
    }
    bdd = manager->bddOne();
}

BDDWrapper::BDDWrapper(int var, int val, bool neg) {
    if(manager == NULL) {
        BDDWrapper::initializeManager();
    }
    bdd = manager->bddVar(fact_ids[var][val]);
    if(neg) {
        bdd = !bdd;
    }
}

void BDDWrapper::initializeManager() {
    fact_ids.resize(g_variable_domain.size());
    int count = 0;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        fact_ids[i].resize(g_variable_domain[i]);
        for(int j = 0; j < g_variable_domain[i]; ++j) {
            fact_ids[i][j]= count++;
        }
    }

    //initializing the char** holding the names of the variables
    fact_names = new char*[count];
    count = 0;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        for(int j = 0; j < g_variable_domain[i]; ++j) {
            fact_names[count] = new char[g_fact_names[i][j].size()+1];
            strcpy(fact_names[count++], g_fact_names[i][j].c_str());
        }
    }

    manager = new Cudd(count,0);
}

void BDDWrapper::dumpBDD(std::string filename, std::string bddname) {
    FILE* f = fopen(filename.c_str(), "w");
    Dddmp_cuddBddStore(manager->getManager(),&bddname[0], bdd.getNode(), fact_names, NULL,
            DDDMP_MODE_TEXT, DDDMP_VARNAMES, &filename[0], f);
    fclose(f);
}

void BDDWrapper::land(int var, int val, bool neg) {
    BDD tmp = manager->bddVar(fact_ids[var][val]);
    if(neg) {
        tmp = !tmp;
    }
    bdd = bdd * tmp;
}

void BDDWrapper::land(BDDWrapper &bdd2) {
    bdd = bdd * bdd2.bdd;
}

void BDDWrapper::lor(int var, int val, bool neg) {
    BDD tmp = manager->bddVar(fact_ids[var][val]);
    if(neg) {
        tmp = !tmp;
    }
    bdd = bdd + tmp;
}

void BDDWrapper::lor(BDDWrapper &bdd2) {
    bdd = bdd + bdd2.bdd;
}

void BDDWrapper::negate() {
    bdd = !bdd;
}

bool BDDWrapper::isOne() {
    return bdd.IsOne();
}

bool BDDWrapper::isZero() {
    return bdd.IsZero();
}

bool BDDWrapper::isEqualTo(BDDWrapper &bdd2) {
    return bdd == bdd2.bdd;
}
#endif
