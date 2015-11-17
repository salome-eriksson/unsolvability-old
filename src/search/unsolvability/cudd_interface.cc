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
    bdd = manager->fact_bdds[var][val];
    if(neg) {
        bdd = !bdd;
    }
}

CuddBDD::CuddBDD(CuddManager *manager, const GlobalState &state)
    : manager(manager) {
    bdd = manager->fact_bdds[0][state[0]];
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        bdd = bdd * manager->fact_bdds[i][state[i]];
    }
}

void CuddBDD::land(int var, int val, bool neg) {
    if(neg) {
        bdd = bdd - manager->fact_bdds[var][val];
    } else {
        bdd = bdd * manager->fact_bdds[var][val];
    }
}

void CuddBDD::lor(int var, int val, bool neg) {
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
    assert(g_variable_domain.size() == var_order.size());

    std::vector<bool> hasNoneOfThose(g_variable_domain.size(), false);
    int amount_vars = 0;

    //collect which variables have a "none of those" value and count total number
    //of PDDL variables
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        std::string last_fact = g_fact_names[i][g_variable_domain[i]-1];
        if(last_fact.substr(0,15).compare("<none of those>") == 0 ||
                last_fact.substr(0,11).compare("NegatedAtom") == 0) {
            hasNoneOfThose[i] = true;
            amount_vars += g_variable_domain[i]-1;
        } else {
            amount_vars += g_variable_domain[i];
        }

    }

    cm = new Cudd(amount_vars,0);

    // fact_bdds represent for each var-val pair (from the search) the correct bdd
    fact_bdds.resize(var_order.size());
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

#endif
