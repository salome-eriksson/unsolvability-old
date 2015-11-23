#include "cudd_interface.h"
#include "../globals.h"

#include <algorithm>

#ifdef USE_CUDD
#include "dddmp.h"


CuddBDD::CuddBDD(CuddManager *manager, bool positive)
    : manager(manager) {
    if(!positive) {
        bdd = manager->cm->bddZero();
    } else {
        bdd = manager->cm->bddOne();
    }
}

CuddBDD::CuddBDD(CuddManager *manager, int var, int val, bool neg)
    : manager(manager) {
    assert(!manager->fact_bdds[var].empty());
    bdd = manager->fact_bdds[var][val];
    if(neg) {
        bdd = !bdd;
    }
}

CuddBDD::CuddBDD(CuddManager *manager, const GlobalState &state)
    : manager(manager) {
    bdd = manager->cm->bddOne();
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        if(!manager->fact_bdds[i].empty()) {
            bdd = bdd * manager->fact_bdds[i][state[i]];
        }
    }
}

void CuddBDD::land(int var, int val, bool neg) {
    assert(!manager->fact_bdds[var].empty());
    if(neg) {
        bdd = bdd - manager->fact_bdds[var][val];
    } else {
        bdd = bdd * manager->fact_bdds[var][val];
    }
}

void CuddBDD::lor(int var, int val, bool neg) {
    assert(!manager->fact_bdds[var].empty());
    if(neg) {
        bdd = bdd + !(manager->fact_bdds[var][val]);
    } else {
        bdd = bdd + manager->fact_bdds[var][val];
    }
}

void CuddBDD::negate() {
    bdd = !bdd;
}

void CuddBDD::land(const CuddBDD &bdd2) {
    bdd = bdd * bdd2.bdd;
}

void CuddBDD::lor(const CuddBDD &bdd2) {
    bdd = bdd + bdd2.bdd;
}

bool CuddBDD::isOne() const {
    return bdd.IsOne();
}

bool CuddBDD::isZero() const {
    return bdd.IsZero();
}

bool CuddBDD::isEqualTo(const CuddBDD &bdd2) const {
    return bdd == bdd2.bdd;
}

CuddManager* CuddBDD::get_manager() {
    return manager;
}

void CuddBDD::dumpBDD(std::string filename, std::string bddname) const {
    FILE* f = fopen(filename.c_str(), "w");
    Dddmp_cuddBddStore(manager->cm->getManager(),&bddname[0], bdd.getNode(), NULL, NULL,
            DDDMP_MODE_TEXT, DDDMP_VARIDS, &filename[0], f);
    fclose(f);
}


CuddManager::CuddManager() {
    std::vector<int> var_order;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        var_order.push_back(i);
    }
    initialize_manager(var_order);
}

CuddManager::CuddManager(std::vector<int> &var_order) {
    initialize_manager(var_order);
}

void CuddManager::initialize_manager(std::vector<int> &var_order) {

    std::vector<bool> hasNoneOfThose(g_variable_domain.size(), false);
    int amount_vars = 0;

    //collect which variables have a "none of those" value and count total number
    //of PDDL variables
    for(size_t i = 0; i < var_order.size(); ++i) {
        int index = var_order[i];
        std::string last_fact = g_fact_names[index][g_variable_domain[index]-1];
        if(last_fact.substr(0,15).compare("<none of those>") == 0 ||
                last_fact.substr(0,11).compare("NegatedAtom") == 0) {
            hasNoneOfThose[index] = true;
            amount_vars += g_variable_domain[index]-1;
        } else {
            amount_vars += g_variable_domain[index];
        }

    }

    cm = new Cudd(amount_vars,0);

    // fact_bdds represent for each var-val pair (from the search) the correct bdd
    fact_bdds.resize(g_variable_domain.size());
    // var_to_fact_pair shows which var-val pair corresponds to which bdd var
    var_to_fact_pair.resize(amount_vars);
    int count = 0;
    for(size_t i = 0; i < var_order.size(); ++i) {
        int index = var_order[i];
        fact_bdds[index] = std::vector<BDD>(g_variable_domain[index], cm->bddOne());
        int pddl_vars = g_variable_domain[index];
        if(hasNoneOfThose[index]) {
            pddl_vars--;
        }
        for(int j = 0; j < pddl_vars; ++j) {
            var_to_fact_pair[count] = std::make_pair(index,j);
            BDD varbdd = cm->bddVar(count++);
            for(int x = 0; x < g_variable_domain[index]; ++x) {
                if(x == j) {
                    fact_bdds[index][x] = fact_bdds[index][x] * varbdd;
                } else {
                    fact_bdds[index][x] = fact_bdds[index][x] - varbdd;
                }
            }
        }
    }
    assert(count == amount_vars);
}

void CuddManager::writeVarOrder(std::ofstream &file) const {
    for(size_t i = 0; i < var_to_fact_pair.size(); ++i) {
        std::pair<int,int> p = var_to_fact_pair[i];
        //substring, because we remove "Atom "
        file << g_fact_names[p.first][p.second].substr(5, g_fact_names[p.first][p.second].size()-5) << "\n";
    }
}

void CuddManager::dumpBDDs(std::vector<CuddBDD*> &bdds, std::vector<std::string> &names, std::string filename) const {
    int size = bdds.size();
    DdNode** bdd_arr = new DdNode*[size];
    char** names_char = new char*[size];
    for(int i = 0; i < size; ++i) {
        bdd_arr[i] = bdds[i]->bdd.getNode();
        names_char[i] = new char[names[i].size() + 1];
        strcpy(names_char[i], names[i].c_str());
    }
    Dddmp_cuddBddArrayStore(cm->getManager(), NULL, size, bdd_arr, names_char,
                            NULL, NULL, DDDMP_MODE_TEXT, DDDMP_VARIDS, &filename[0], NULL);
}

#endif
