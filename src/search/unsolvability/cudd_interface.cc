#include "cudd_interface.h"
#include "../globals.h"

#ifdef USE_CUDD
#include "dddmp.h"

std::vector<std::vector<BDD> > BDDWrapper::fact_bdds = std::vector<std::vector<BDD> >();
Cudd* BDDWrapper::manager = NULL;
char** BDDWrapper::fact_names = NULL;
std::vector<std::string> BDDWrapper::fact_names_str = std::vector<std::string>();


BDDWrapper::BDDWrapper() {
    if(manager == NULL) {
        std::cout << "Creating a BDD without initializing variable order - using default order"
                     << std::endl;
        BDDWrapper::initializeManager(std::vector<int>());
    }
    bdd = manager->bddOne();
}

BDDWrapper::BDDWrapper(bool one) {
    if(manager == NULL) {
        std::cout << "Creating a BDD without initializing variable order - using default order"
                     << std::endl;
        BDDWrapper::initializeManager(std::vector<int>());
    }
    if(one) {
        bdd = manager->bddOne();
    } else {
        bdd = manager->bddZero();
    }
}

BDDWrapper::BDDWrapper(int var, int val, bool neg) {
    if(manager == NULL) {
        std::cout << "Creating a BDD without initializing variable order - using default order"
                     << std::endl;
        BDDWrapper::initializeManager(std::vector<int>());
    }
    bdd = fact_bdds[var][val];
    if(neg) {
        bdd = !bdd;
    }
}

void BDDWrapper::initializeManager(std::vector<int> var_order) {
    assert(manager == NULL);
    if(var_order.empty()) {
        for(size_t i = 0; i < g_variable_domain.size(); ++i) {
            var_order.push_back(i);
        }
    }
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

    manager = new Cudd(amount_vars,0);

    fact_bdds.resize(var_order.size());
    int count = 0;
    for(size_t i = 0; i < var_order.size(); ++i) {
        int index = var_order[i];
        fact_bdds[index] = std::vector<BDD>(g_variable_domain[index], manager->bddOne());
        int domain_size = g_variable_domain[index];
        if(hasNoneOfThose[index]) {
            domain_size--;
        }
        for(int j = 0; j < domain_size; ++j) {
            BDD varbdd = manager->bddVar(count++);
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

    //initializing the char** holding the names of the variables
    fact_names = new char*[amount_vars];
    fact_names_str = std::vector<std::string>(amount_vars);
    count = 0;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        int index = var_order[i];
        for(int j = 0; j < g_variable_domain[index]; ++j) {
            if(j != g_variable_domain[index]-1 || !hasNoneOfThose[index]) {
                fact_names[count] = new char[g_fact_names[index][j].size()+1];
                fact_names_str[count] = g_fact_names[index][j];
                strcpy(fact_names[count++], g_fact_names[index][j].c_str());
            }
        }
    }
    assert(count == amount_vars);

}

void BDDWrapper::dumpBDD(std::string filename, std::string bddname) const {
    FILE* f = fopen(filename.c_str(), "w");
    Dddmp_cuddBddStore(manager->getManager(),&bddname[0], bdd.getNode(), fact_names, NULL,
            DDDMP_MODE_TEXT, DDDMP_VARNAMES, &filename[0], f);
    fclose(f);
}

void BDDWrapper::writeVarOrder(std::ofstream &file) const {
    for(size_t i = 0; i < fact_names_str.size(); ++i) {
        file << fact_names_str[i]; << "|"
    }
    file << "\n";
}

void BDDWrapper::land(int var, int val, bool neg) {
    BDD tmp = fact_bdds[var][val];
    if(neg) {
        tmp = !tmp;
    }
    bdd = bdd * tmp;
}

void BDDWrapper::land(const BDDWrapper &bdd2) {
    bdd = bdd * bdd2.bdd;
}

void BDDWrapper::lor(int var, int val, bool neg) {
    BDD tmp = fact_bdds[var][val];
    if(neg) {
        tmp = !tmp;
    }
    bdd = bdd + tmp;
}

void BDDWrapper::lor(const BDDWrapper &bdd2) {
    bdd = bdd + bdd2.bdd;
}

void BDDWrapper::negate() {
    bdd = !bdd;
}

bool BDDWrapper::isOne() const{
    return bdd.IsOne();
}

bool BDDWrapper::isZero() const{
    return bdd.IsZero();
}

bool BDDWrapper::isEqualTo(const BDDWrapper &bdd2) const{
    return bdd == bdd2.bdd;
}
#endif
