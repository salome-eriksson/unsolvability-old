#include "cudd_interface.h"
#include "../globals.h"
#include "../global_operator.h"

#include <algorithm>

#ifdef USE_CUDD
#include "dddmp.h"

using Utils::ExitCode;

std::vector<int> CuddManager::var_order = std::vector<int>();
CuddManager* CuddManager::instance = NULL;

void exit_oom(long size) {
    Utils::exit_with(ExitCode::OUT_OF_MEMORY);
}

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

CuddBDD::CuddBDD(CuddManager *manager,const std::vector<std::pair<int,int> >& pos_facts,
                 const std::vector<std::pair<int,int> > &neg_facts)
    : manager(manager) {
    std::vector<int> cube(manager->amount_vars, 2);
    for(size_t i = 0; i < pos_facts.size(); ++i) {
        const std::pair<int,int>& p = pos_facts[i];
        assert(p.first < (int)manager->fact_to_var.size()
               && p.second < (int)manager->fact_to_var[p.first].size());
        cube[manager->fact_to_var[p.first][p.second]] = 1;
    }
    for(size_t i = 0; i < neg_facts.size(); ++i) {
        const std::pair<int,int>& p = neg_facts[i];
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

CuddManager::CuddManager(std::vector<int> &var_order) {
    assert(var_order.size() == g_variable_domain.size());
    fact_to_var.resize(g_variable_domain.size(), std::vector<int>());
    amount_vars = 0;
    for(size_t i = 0; i < var_order.size(); ++i) {
        int index = var_order[i];
        fact_to_var[index].resize(g_variable_domain[index]);
        for(int j = 0; j < g_variable_domain[index]; ++j) {
            fact_to_var[index][j] = amount_vars++;
        }
    }
    ddmgr = Cudd_Init(amount_vars,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0);
    extern DD_OOMFP MMoutOfMemory;
    MMoutOfMemory = exit_oom;
}

void CuddManager::set_variable_order(std::vector<int> &_var_order) {
    if(!var_order.empty()) {
        std::cerr << "Tried to set variable order twice" << std::endl;
        Utils::exit_with(Utils::ExitCode::CRITICAL_ERROR);
    }
    assert(_var_order.size() == g_variable_domain.size());
    var_order = _var_order;
}

CuddManager* CuddManager::get_instance() {
    if(var_order.empty()) {
        std::cerr << "Tried to create manager without setting variable order" << std::endl;
        Utils::exit_with(Utils::ExitCode::CRITICAL_ERROR);
    }
    if(!instance) {
        instance = new CuddManager(var_order);
    }
    return instance;
}

void CuddManager::writeTaskFile() const{
    std::ofstream task_file;
    task_file.open("task.txt");

    int fact_amount = 0;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        fact_amount += g_variable_domain[i];
    }
    task_file << "begin_atoms:" << fact_amount << "\n";
    for(size_t i = 0; i < var_order.size(); ++i) {
        int var = var_order[i];
        for(size_t j = 0; j < g_fact_names[var].size(); ++j) {
            task_file << g_fact_names[var][j] << "\n";
        }
    }
    task_file << "end_atoms\n";

    task_file << "begin_init\n";
    for(size_t i = 0; i < g_fact_names.size(); ++i) {
        task_file << fact_to_var[i][g_initial_state()[i]] << "\n";
    }
    task_file << "end_init\n";

    task_file << "begin_goal\n";
    for(size_t i = 0; i < g_goal.size(); ++i) {
        task_file << fact_to_var[g_goal[i].first][g_goal[i].second] << "\n";
    }
    task_file << "end_goal\n";


    task_file << "begin_actions:" << g_operators.size() << "\n";
    for(size_t op_index = 0;  op_index< g_operators.size(); ++op_index) {
        const GlobalOperator& op = g_operators[op_index];
        task_file << "begin_action\n"
                  << op.get_name() << "\n"
                  << "cost: "<< op.get_cost() <<"\n";
        const std::vector<GlobalCondition>& pre = op.get_preconditions();
        const std::vector<GlobalEffect>& post = op.get_effects();
        for(size_t i = 0; i < pre.size(); ++i) {
            task_file << "PRE:" << fact_to_var[pre[i].var][pre[i].val] << "\n";
        }
        for(size_t i = 0; i < post.size(); ++i) {
            if(!post[i].conditions.empty()) {
                std::cout << "CONDITIONAL EFFECTS, ABORT!";
                std::exit(1);
            }
            task_file << "ADD:" << fact_to_var[post[i].var][post[i].val] << "\n";
            // all other facts from this FDR variable are set to false
            // TODO: can we make this more compact / smarter?
            for(int j = 0; j < g_variable_domain[post[i].var]; j++) {
                if(j == post[i].val) {
                    continue;
                }
                task_file << "DEL:" << fact_to_var[post[i].var][j] << "\n";
            }
        }
        task_file << "end_action\n";
    }
    task_file << "end_actions\n";
    task_file.close();
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
