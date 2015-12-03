#ifndef CUDD_INTERFACE_H
#define CUDD_INTERFACE_H


#include <functional>
#include <memory>
#include <vector>

#include "../global_state.h"
#include "../utilities.h"

#include <iostream>
#include <fstream>


/*
  All methods that use CUDD specific classes only do something useful
  if the planner is compiled with USE_CUDD. Otherwise, they just print
  an error message and abort.
*/
#ifdef USE_CUDD
#include "cudd.h"
#define CUDD_METHOD(X) X;
#else
class DdNode {};
#define CUDD_METHOD(X) NO_RETURN X { \
        ABORT("CUDD method called but the planner was compiled without CUDD support.\n"); \
}
#endif

class Cudd;

#ifdef __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

class CuddManager;

class CuddBDD {
    friend class CuddManager;
private:
    CuddManager *manager;
    DdNode* bdd;
public:
    CUDD_METHOD(CuddBDD())
    CUDD_METHOD(CuddBDD(CuddManager *manager, bool positive=true))
    CUDD_METHOD(CuddBDD(CuddManager *manager, int var, int val, bool neg=false))
    CUDD_METHOD(CuddBDD(CuddManager *manager, const GlobalState &state))
    //create a cube where the facts denoted by pos_vars are true
    //and the facts denoted by neg_vars are false
    CUDD_METHOD(CuddBDD(CuddManager *manager, const std::vector<std::pair<int,int> >& pos_vars,
                        const std::vector<std::pair<int,int> > &neg_vars))
    CUDD_METHOD(CuddBDD(const CuddBDD& from))
    CUDD_METHOD(~CuddBDD())
    CUDD_METHOD(CuddBDD operator=(const CuddBDD& right))
    CUDD_METHOD(void land(int var, int val, bool neg = false))
    CUDD_METHOD(void lor(int var, int val, bool neg = false))
    CUDD_METHOD(void negate())
    CUDD_METHOD(void land(const CuddBDD &bdd2))
    CUDD_METHOD(void lor(const CuddBDD &bdd2))

    CUDD_METHOD(bool isOne() const)
    CUDD_METHOD(bool isZero() const)

    CUDD_METHOD(bool isEqualTo(const CuddBDD &bdd2) const)

    //TODO: not sure if this is a good idea, it is used in heuristic representation in M&S
    //to get the M&S manager without needing to pass it through all the time
    CUDD_METHOD(CuddManager *get_manager())

    CUDD_METHOD(void dumpBDD(std::string filename, std::string bddname) const)
};

class CuddManager {
    friend class CuddBDD;
private:
#ifdef USE_CUDD
    DdManager* ddmgr;
    std::vector<std::pair<int,int> > var_to_fact_pair;
    std::vector<std::vector<DdNode*> > fact_bdds;
    //saves to which bdd var a fact corresponds. If the fact is a "negated atom"
    //or "none of those", -1 is saved as value
    std::vector<std::vector<int> > fact_to_bdd_var;
    int amount_vars;
#endif
    CUDD_METHOD(void initialize_manager(std::vector<int> &var_order))
public:
    CUDD_METHOD(CuddManager())
    CUDD_METHOD(CuddManager(std::vector<int> &var_order))

    CUDD_METHOD(void writeVarOrder(std::ofstream &file) const)
    CUDD_METHOD(void dumpBDDs(std::vector<CuddBDD*> &bdds, std::vector<std::string> &names, std::string filename) const)
};

#endif
