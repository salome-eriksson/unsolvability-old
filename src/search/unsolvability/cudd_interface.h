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
        ABORT("LP method called but the planner was compiled without LP support.\n" \
              "See http://www.fast-downward.org/LPBuildInstructions\n" \
              "to install an LP solver and use it in the planner."); \
}
#endif

class Cudd;

class BDDWrapper {
private:
#ifdef USE_CUDD
    BDD bdd;
    static Cudd* manager;
    static std::vector<std::vector<int> > fact_ids;
    static char** fact_names;
#endif
    CUDD_METHOD(static void initializeManager())
public:
    CUDD_METHOD(BDDWrapper())
    CUDD_METHOD(BDDWrapper(int var, int val, bool neg = false))
    CUDD_METHOD(void dumpBDD(std::string filename, std::string bddname))
    /*CUDD_METHOD(void land(int var, int val, bool neg = false))
    CUDD_METHOD(void lor(int var, int val, bool neg = false))
    CUDD_METHOD(void negate())
    CUDD_METHOD(void land(BDDWrapper* bdd2))
    CUDD_METHOD(void lor(BDDWrapper* bdd2))
    CUDD_METHOD(bool isEmpty())
    CUDD_METHOD(bool isOne())
    CUDD_METHOD(bool isZero())

    CUDD_METHOD(static bool areEqual(BDDWrapper* bdd1, BDDWrapper* bdd2))*/
};

#endif
