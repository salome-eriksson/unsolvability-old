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
    assert(var < (int)manager->fact_to_var.size() && val < (int)manager->fact_to_var[var].size());
    bdd = Cudd_bddIthVar(manager->ddmgr, manager->fact_to_var[var][val]);
    if(neg) {
        bdd = Cudd_Not(bdd);
    }
    Cudd_Ref(bdd);
}

CuddBDD::CuddBDD(CuddManager *manager, const GlobalState &state)
    : manager(manager) {
    std::vector<int> cube(manager->amount_vars, 0);
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        cube[manager->fact_to_var[i][state[i]]] = 1;
    }
    bdd = Cudd_CubeArrayToBdd(manager->ddmgr, &cube[0]);
    Cudd_Ref(bdd);
}

CuddBDD::CuddBDD(CuddManager *manager,const std::vector<std::pair<int,int> >& pos_vars,
                 const std::vector<std::pair<int,int> > &neg_vars)
    : manager(manager) {
    std::vector<int> cube(manager->amount_vars, 2);
    for(size_t i = 0; i < pos_vars.size(); ++i) {
        const std::pair<int,int>& p = pos_vars[i];
        assert(p.first < (int)manager->fact_to_var.size()
               && p.second < (int)manager->fact_to_var[p.first].size());
        cube[manager->fact_to_var[p.first][p.second]] = 1;
    }
    for(size_t i = 0; i < neg_vars.size(); ++i) {
        const std::pair<int,int>& p = neg_vars[i];
        assert(p.first < (int)manager->fact_to_var.size()
               && p.second < (int)manager->fact_to_var[p.first].size());
        cube[manager->fact_to_var[p.first][p.second]] = 0;
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
    assert(var < (int)manager->fact_to_var.size() && val < (int)manager->fact_to_var[var].size());
    DdNode* tmp;
    if(neg) {
        tmp = Cudd_bddAnd(manager->ddmgr, bdd, Cudd_Not(Cudd_bddIthVar(manager->ddmgr, manager->fact_to_var[var][val])));
    } else {
        tmp = Cudd_bddAnd(manager->ddmgr, bdd, Cudd_bddIthVar(manager->ddmgr, manager->fact_to_var[var][val]));
    }
    Cudd_Ref(tmp);
    Cudd_RecursiveDeref(manager->ddmgr, bdd);
    bdd = tmp;
}

void CuddBDD::lor(int var, int val, bool neg) {
    assert(var < (int)manager->fact_to_var.size() && val < (int)manager->fact_to_var[var].size());
    DdNode* tmp;
    if(neg) {
        tmp = Cudd_bddOr(manager->ddmgr, bdd, Cudd_Not(Cudd_bddIthVar(manager->ddmgr, manager->fact_to_var[var][val])));
    } else {
        tmp = Cudd_bddOr(manager->ddmgr, bdd, Cudd_bddIthVar(manager->ddmgr, manager->fact_to_var[var][val]));
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

bool CuddBDD::isSubsetOf(const CuddBDD &bdd2) const {
    assert(manager->ddmgr == bdd2.manager->ddmgr);
    return (Cudd_bddLeq(manager->ddmgr, bdd, bdd2.bdd) == 1);
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
    int count = 0;
    std::vector<std::vector<int>> var_order(g_variable_domain.size());
    for(size_t i = 0; i < var_order.size(); ++i) {
        var_order[i] = std::vector<int>(g_variable_domain[i]);
        for(size_t j = 0; j < var_order[i].size(); ++j) {
            var_order[i][j] = count++;
        }
    }
    initialize_manager(var_order);
}

CuddManager::CuddManager(std::vector<int> &var_order) {
    assert(var_order.size() == g_variable_domain.size());
    int count = 0;
    std::vector<std::vector<int>> detailed_var_order(g_variable_domain.size());
    for(size_t i = 0; i < var_order.size(); ++i) {
        assert(var_order[i] < (int)g_variable_domain.size()
               && detailed_var_order[var_order[i]].empty());
        for(int j = 0; j < g_variable_domain[var_order[i]]; ++j) {
            detailed_var_order[var_order[i]].push_back(count++);
        }
    }
    initialize_manager(detailed_var_order);
}

CuddManager::CuddManager(std::vector<std::vector<int>> &var_order) {
    initialize_manager(var_order);
}

void CuddManager::initialize_manager(std::vector<std::vector<int>> &var_order) {
    //assert var_order has the right dimensions and count total amount of facts
    assert(var_order.size() == g_variable_domain.size());
    amount_vars = 0;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        assert((int)var_order[i].size() == g_variable_domain[i]);
        amount_vars += g_variable_domain[i];
    }

    var_to_fact.resize(amount_vars, std::pair<int,int>(-1,-1));
    int assigned_vars = 0;

    // count how many variables are assigned in the vector and ensure that
    // none are assigned the same number
    for(size_t i = 0; i < var_order.size(); ++i) {
        for(size_t j = 0; j < var_order[i].size(); ++j) {
            int x = var_order[i][j];
            if(var_order[i][j] >= 0) {
                assigned_vars++;
                assert(var_to_fact[x].first == -1);
                var_to_fact[x] = std::pair<int,int>(i,j);
            }
        }
    }
    // copy var_order
    // TODO: check if this is an actual copy
    fact_to_var = var_order;

    // fill in unassigned variables
    for(size_t i = 0; i < fact_to_var.size(); i++) {
        for(size_t j = 0; j < fact_to_var[i].size(); j++) {
            if(fact_to_var[i][j] < 0) {
                fact_to_var[i][j] = assigned_vars;
                var_to_fact[assigned_vars] = std::pair<int,int>(i,j);
                assigned_vars++;
            }
        }
    }
    assert(assigned_vars == amount_vars);

    ddmgr = Cudd_Init(amount_vars,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0);
}

void CuddManager::writeVarOrder(std::ofstream &file) const {
    for(size_t i = 0; i < fact_to_var.size(); ++i) {
        for(size_t j = 0; j < fact_to_var[i].size(); ++j) {
            file << fact_to_var[i][j] << "\n";
        }
    }
    /*for(size_t i = 0; i < var_to_fact.size(); ++i) {
        file << var_to_fact[i].first << "," << var_to_fact[i].second << "\n";
    }*/
}

void CuddManager::dumpBDDs(std::vector<CuddBDD*> &bdds, std::vector<std::string> &names, std::string filename) const {
    int size = bdds.size();
    DdNode** bdd_arr = new DdNode*[size];
    char** names_char = new char*[size];
    for(int i = 0; i < size; ++i) {
        bdd_arr[i] = bdds[i]->bdd;
        names_char[i] = new char[names[i].size() + 1];
        strcpy(names_char[i], names[i].c_str());
    }
    Dddmp_cuddBddArrayStore(ddmgr, NULL, size, &bdd_arr[0], names_char,
                            NULL, NULL, DDDMP_MODE_TEXT, DDDMP_VARIDS, &filename[0], NULL);
}

#endif
