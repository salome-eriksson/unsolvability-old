#include "cudd_interface.h"
#include "../globals.h"

#include <algorithm>

#ifdef USE_CUDD
#include "dddmp.h"

CuddBDD::CuddBDD() : manager(NULL), bdd(NULL) {}

CuddBDD::CuddBDD(CuddManager *manager, bool positive)
    : manager(manager) {
    if(positive) {
        bdd = Cudd_ReadOne(manager->ddmgr);
    } else {
        bdd = Cudd_ReadLogicZero(manager->ddmgr);
    }
    Cudd_Ref(bdd);
}

CuddBDD::CuddBDD(CuddManager *manager, int var, int val, bool neg)
    : manager(manager) {
    assert(!manager->fact_bdds[var].empty());
    bdd = manager->fact_bdds[var][val];
    if(neg) {
        bdd = Cudd_Not(bdd);
    }
    Cudd_Ref(bdd);
}

CuddBDD::CuddBDD(CuddManager *manager, const GlobalState &state)
    : manager(manager) {
    std::vector<int> cube(manager->amount_vars, 0);
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        int bdd_var = manager->fact_to_bdd_var[i][state[i]];
        if(bdd_var != -1) {
            cube[bdd_var] = 1;
        }
    }
    bdd = Cudd_CubeArrayToBdd(manager->ddmgr, &cube[0]);
    Cudd_Ref(bdd);
}

CuddBDD::~CuddBDD() {
    Cudd_RecursiveDeref(manager->ddmgr, bdd);
}
//TODO: here and in operator= check why the manager pointer is not null sometimes
//(example: vector initialization with (amount, CuddBdd(manager, true)), concrete in heuristic_representation line 119)
CuddBDD::CuddBDD(const CuddBDD &from) {
    manager = from.manager;
    bdd = from.bdd;
    Cudd_Ref(bdd);
}

CuddBDD CuddBDD::operator=(const CuddBDD& right) {
    if (this == &right) {
        return *this;
    }
    manager = right.manager;
    Cudd_Ref(right.bdd);
    Cudd_RecursiveDeref(manager->ddmgr,bdd);
    bdd = right.bdd;
    return *this;

}

void CuddBDD::land(int var, int val, bool neg) {
    assert(!manager->fact_bdds[var].empty());
    DdNode* tmp;
    if(neg) {
        tmp = Cudd_bddAnd(manager->ddmgr, bdd, Cudd_Not(manager->fact_bdds[var][val]));
    } else {
        tmp = Cudd_bddAnd(manager->ddmgr, bdd, manager->fact_bdds[var][val]);
    }
    Cudd_Ref(tmp);
    Cudd_RecursiveDeref(manager->ddmgr, bdd);
    bdd = tmp;
}

void CuddBDD::lor(int var, int val, bool neg) {
    assert(!manager->fact_bdds[var].empty());
    DdNode* tmp;
    if(neg) {
        tmp = Cudd_bddOr(manager->ddmgr, bdd, Cudd_Not(manager->fact_bdds[var][val]));
    } else {
        tmp = Cudd_bddOr(manager->ddmgr, bdd, manager->fact_bdds[var][val]);
    }
    Cudd_Ref(tmp);
    Cudd_RecursiveDeref(manager->ddmgr, bdd);
    bdd = tmp;
}

void CuddBDD::negate() {
    DdNode* tmp = Cudd_Not(bdd);
    //TODO: is this necessary here?
    Cudd_Ref(tmp);
    Cudd_RecursiveDeref(manager->ddmgr, bdd);
    bdd = tmp;
}

void CuddBDD::land(const CuddBDD &bdd2) {
    DdNode* tmp = Cudd_bddAnd(manager->ddmgr, bdd ,bdd2.bdd);
    Cudd_Ref(tmp);
    Cudd_RecursiveDeref(manager->ddmgr, bdd);
    bdd = tmp;
}

void CuddBDD::lor(const CuddBDD &bdd2) {
    DdNode* tmp = Cudd_bddOr(manager->ddmgr, bdd, bdd2.bdd);
    Cudd_Ref(tmp);
    Cudd_RecursiveDeref(manager->ddmgr, bdd);
    bdd = tmp;
}

bool CuddBDD::isOne() const {
    return (bdd == Cudd_ReadOne(manager->ddmgr));
}

bool CuddBDD::isZero() const {
    return (bdd == Cudd_ReadLogicZero(manager->ddmgr));
}

bool CuddBDD::isEqualTo(const CuddBDD &bdd2) const {
    return bdd == bdd2.bdd;
}

CuddManager* CuddBDD::get_manager() {
    return manager;
}

void CuddBDD::dumpBDD(std::string filename, std::string bddname) const {
    FILE* f = fopen(filename.c_str(), "w");
    Dddmp_cuddBddStore(manager->ddmgr,&bddname[0], bdd, NULL, NULL,
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
    amount_vars = 0;

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

    ddmgr = Cudd_Init(amount_vars,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0);

    // fact_bdds represent for each var-val pair (from the search) the correct bdd
    fact_bdds.resize(g_variable_domain.size());
    // var_to_fact_pair shows which var-val pair corresponds to which bdd var
    var_to_fact_pair.resize(amount_vars);
    fact_to_bdd_var.resize(g_variable_domain.size());
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        fact_to_bdd_var[i] = std::vector<int>(g_variable_domain[i], -1);
    }
    int count = 0;
    for(size_t i = 0; i < var_order.size(); ++i) {
        int index = var_order[i];
        fact_bdds[index] = std::vector<DdNode*>(g_variable_domain[index], Cudd_ReadOne(ddmgr));
        int pddl_vars = g_variable_domain[index];
        if(hasNoneOfThose[index]) {
            pddl_vars--;
        }
        for(int j = 0; j < pddl_vars; ++j) {
            var_to_fact_pair[count] = std::make_pair(index,j);
            fact_to_bdd_var[index][j] = count;
            for(int x = 0; x < g_variable_domain[index]; ++x) {
                DdNode* tmp;
                if(x == j) {
                    tmp = Cudd_bddAnd(ddmgr, fact_bdds[index][x], Cudd_bddIthVar(ddmgr, count));
                } else {
                    tmp = Cudd_bddAnd(ddmgr, fact_bdds[index][x], Cudd_Not(Cudd_bddIthVar(ddmgr, count)));
                }
                Cudd_Ref(tmp);
                Cudd_RecursiveDeref(ddmgr, fact_bdds[index][x]);
                fact_bdds[index][x] = tmp;
            }
            count++;
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
        bdd_arr[i] = bdds[i]->bdd;
        if(bdds[i]->isOne()) {
            std::cout << "is one" << std::endl;
        }
        names_char[i] = new char[names[i].size() + 1];
        strcpy(names_char[i], names[i].c_str());
    }
    Dddmp_cuddBddArrayStore(ddmgr, NULL, size, &bdd_arr[0], names_char,
                            NULL, NULL, DDDMP_MODE_TEXT, DDDMP_VARIDS, &filename[0], NULL);
}

#endif
