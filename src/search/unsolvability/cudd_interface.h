#ifndef CUDD_INTERFACE_H
#define CUDD_INTERFACE_H


#include <functional>
#include <memory>
#include <vector>

#include "../utilities.h"


/*
  All methods that use CUDD specific classes only do something useful
  if the planner is compiled with USE_CUDD. Otherwise, they just print
  an error message and abort.
*/
#ifdef USE_CUDD
#include "cuddObj.hh"
#define CUDD_METHOD(X) X;
#else
class BDD {};
#define CUDD_METHOD(X) NO_RETURN X { \
        ABORT("CUDD method called but the planner was compiled without CUDD support.\n"); \
}
#endif

class Cudd;

#ifdef __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
class BDDWrapper {
private:
#ifdef USE_CUDD
    BDD bdd;
    static Cudd* manager;
    static std::vector<std::vector<BDD> > fact_bdds;
    static char** fact_names;
#endif
public:
    CUDD_METHOD(BDDWrapper())
    CUDD_METHOD(BDDWrapper(bool one))
    CUDD_METHOD(BDDWrapper(int var, int val, bool neg = false))
    CUDD_METHOD(void dumpBDD(std::string filename, std::string bddname) const)

    CUDD_METHOD(void land(int var, int val, bool neg = false))
    CUDD_METHOD(void lor(int var, int val, bool neg = false))
    CUDD_METHOD(void negate())
    CUDD_METHOD(void land(const BDDWrapper &bdd2))
    CUDD_METHOD(void lor(const BDDWrapper &bdd2))

    CUDD_METHOD(bool isOne() const)
    CUDD_METHOD(bool isZero() const)

    CUDD_METHOD(bool isEqualTo(const BDDWrapper &bdd2) const)

    //this method should only be called if no BDD has been created yet
    //(this is guarded by an assertion)
    CUDD_METHOD(static void initializeManager(std::vector<int> var_order))
};

#endif
