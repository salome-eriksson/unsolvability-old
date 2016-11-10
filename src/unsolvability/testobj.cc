/**CFile***********************************************************************

  FileName    [testobj.cc]

  PackageName [cuddObj]

  Synopsis    [Test program for the C++ object-oriented encapsulation of CUDD.]

  Description []

  SeeAlso     []

  Author      [Fabio Somenzi]

  Copyright   [Copyright (c) 1995-2012, Regents of the University of Colorado

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  Neither the name of the University of Colorado nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.]

******************************************************************************/
#include "dddmp.h"
#include "cuddObj.hh"
#include <math.h>
#include <iostream>
#include <cassert>
#include <stdio.h>


using namespace std;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void testBdd(Cudd& mgr, int verbosity);
static void testAdd(Cudd& mgr, int verbosity);
static void testAdd2(Cudd& mgr, int verbosity);
static void testZdd(Cudd& mgr, int verbosity);
static void testBdd2(Cudd& mgr, int verbosity);
static void testBdd3(Cudd& mgr, int verbosity);
static void testZdd2(Cudd& mgr, int verbosity);
static void testBdd4(Cudd& mgr, int verbosity);
static void testBdd5(Cudd& mgr, int verbosity);


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Main program for testobj.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int
main(int argc, char **argv)
{
    FILE *fp;
    fp=fopen("states.bdd", "r");
    int numvars = 42;
    int compose[numvars];
    for(int i = 0; i < numvars; ++i) {
        compose[i] = 2*i;
    }
    Cudd mgr(numvars*2,0);
    std::cout << "created manager" << std::endl;
    BDD x = BDD(mgr, Dddmp_cuddBddLoad(mgr.getManager(), 
      DDDMP_VAR_COMPOSEIDS, NULL, NULL, &compose[0], DDDMP_MODE_TEXT, NULL, fp));
    std::cout << "Read in first bdd" << std::endl;
      
    BDD y = mgr.bddOne();
    y = y - mgr.bddVar(0);
    y = y - mgr.bddVar(2);
    y = y - mgr.bddVar(4);
    y = y - mgr.bddVar(6);
    y = y - mgr.bddVar(8);
    y = y * mgr.bddVar(10);
    y = y * mgr.bddVar(12);
    y = y - mgr.bddVar(14);
    y = y - mgr.bddVar(16);
    y = y * mgr.bddVar(18);
    y = y * mgr.bddVar(20);
    y = y - mgr.bddVar(22);
    y = y * mgr.bddVar(24);
    y = y - mgr.bddVar(26);
    y = y - mgr.bddVar(28);
    y = y * mgr.bddVar(30);
    y = y * mgr.bddVar(32);
    y = y - mgr.bddVar(34);
    y = y - mgr.bddVar(36);
    y = y - mgr.bddVar(38);
    y = y - mgr.bddVar(40);
    y = y * mgr.bddVar(42);
    y = y - mgr.bddVar(44);
    y = y - mgr.bddVar(46);
    y = y - mgr.bddVar(48);
    y = y * mgr.bddVar(50);
    y = y - mgr.bddVar(52);
    y = y - mgr.bddVar(54);
    y = y - mgr.bddVar(56);
    y = y - mgr.bddVar(58);
    y = y - mgr.bddVar(60);
    y = y - mgr.bddVar(62);
    y = y - mgr.bddVar(64);
    y = y - mgr.bddVar(66);
    y = y - mgr.bddVar(68);
    y = y * mgr.bddVar(70);
    y = y - mgr.bddVar(72);
    y = y - mgr.bddVar(74);
    y = y - mgr.bddVar(76);
    y = y - mgr.bddVar(78);
    y = y - mgr.bddVar(80);
    y = y * mgr.bddVar(82);
    
    if(x.Leq(y)) {
        std::cout << "x leq y" << std::endl;
    }
    if(y.Leq(x)) {
        std::cout << "y leq x" << std::endl;
    }
    if(x == y) {
        std::cout << "IT WORKED!!!" << std::endl;
    }
    /*BDD a = BDD(mgr, Dddmp_cuddBddLoad(mgr.getManager(), 
      DDDMP_VAR_COMPOSEIDS, NULL, NULL, &compose[0], DDDMP_MODE_TEXT, NULL, fp));
    std::cout << "Read in second bdd" << std::endl;
    BDD b = mgr.bddVar(0);
    b = b - mgr.bddVar(2);
    b = b - mgr.bddVar(4);
    b = b * mgr.bddVar(6);
    b = b - mgr.bddVar(8);
    b = b * mgr.bddVar(10);
    b = b - mgr.bddVar(12);
    b = b - mgr.bddVar(14);
    b = b * mgr.bddVar(16);
    b = b - mgr.bddVar(18);
    if(a.Leq(b)) {
        std::cout << "x leq y" << std::endl;
    }
    if(b.Leq(a)) {
        std::cout << "y leq x" << std::endl;
    }
    if(a == b) {
        std::cout << "IT WORKED!!!" << std::endl;
    }*/
    exit(0);
    /*int verbosity = 1;

    if (argc == 2) {
        int retval = sscanf(argv[1], "%d", &verbosity);
        if (retval != 1)
            return 1;
    } else if (argc != 1) {
        return 1;
    }
    
    Cudd mgr(3,0);
    BDD v1 = mgr.bddVar(0);
    BDD v2= mgr.bddVar(1);
    BDD v3 = mgr.bddVar(2);
    
    BDD x = (v1 + v2) - v3;
    BDD y = x + (!v2 * v3);
    
    if(x.Leq(y)) {
      std::cout << "x leq y" << std::endl;
    } else {
      std::cout << "?????" << std::endl;
    }
    
    if(y.Leq(x)) {
      std::cout << "y leq x???" << std::endl;
    }
    
    BDD z = !x;
    z = z * !v1;
    
    if(x.Leq(!z)) {
      std::cout << "they are disjunct!" << std::endl;
    } else {
      std::cout << "not disjunct??" << std::endl;
    }
    
    BDD x1 = x * z;
    if(x1.IsZero()) {
      std::cout << "intersect is empty" << std::endl;
    }
    
    BDD x2 = x - z;
    if(x2 == x) {
      std::cout << "they are equal" << std::endl;
    } else {
      std::cout << "why not equal??" << std::endl;
    }
    
    BDD t1 = v1*v2*v3;
    BDD t2 = t1 + (v1-v2-v3);
    BDD t3 = t1 + (v1*v2-v3);
    
    if(t2.Leq(t3) || t3.Leq(t2)) {
      std::cout << "hmmmmmm...." << std::endl;
    }
    
    x = v1 + (!v1 * v2);
    y = (!v1 * v3) + !v2;
    
    if(y.Leq(x)) {
       std::cout << "yay" << std::endl;
    } else {
       std::cout << "nejjj!" << std::endl;
    }
    BDD intersect = x.Intersect(y);
    if(!intersect.IsZero()) {
       std::cout << "they intersect" << std::endl;
       std::cout << intersect.FactoredFormString() << std::endl;
    }
    BDD res = x*y;
    if(!res.IsZero()) {
       std::cout << "yup, they intersect alright" << std::endl;
       std::cout << res.FactoredFormString() << std::endl;
    }*/
    
    
    /*BDD a = (v1+!v2)*v3;
    std::vector<std::string> names;
    names.push_back("a");
    names.push_back("b");
    names.push_back("c");
    
    char ** nameschar = new char*[names.size()];
    for(size_t i = 0; i < names.size(); i++){
        nameschar[i] = new char[names[i].size() + 1];
        strcpy(nameschar[i], names[i].c_str());
    }
    
    std::string bddname = "testbdd";
    std::string outfile = "bdd.txt";
    
    std::cout << "storing 1" << std::endl;
    
    Dddmp_cuddBddStore(mgr.getManager(),&bddname[0], a.getNode(), nameschar, NULL, DDDMP_MODE_TEXT, DDDMP_VARNAMES, &outfile[0], NULL);
    Cudd mgr2(6,0);
    
    int compose[3];
    compose[0]=3;
    compose[1]=5;
    compose[2]=1;

    names.clear();
    names.push_back("b");
    names.push_back("b'");
    names.push_back("c");
    names.push_back("c'");
    names.push_back("a");
    names.push_back("a'");

    nameschar = new char*[names.size()];
        for(size_t i = 0; i < names.size(); i++){
            nameschar[i] = new char[names[i].size() + 1];
            strcpy(nameschar[i], names[i].c_str());
        }
    
    std::cout << "loading 1" << std::endl;
    BDD x = BDD(mgr2, Dddmp_cuddBddLoad(mgr2.getManager(), 
      DDDMP_VAR_COMPOSEIDS, NULL, NULL, &compose[0], DDDMP_MODE_TEXT, &outfile[0], NULL));
      
    outfile = "bdd2.txt";
    
    std::cout << "storing 2" << std::endl;
    Dddmp_cuddBddStore(mgr2.getManager(),&bddname[0], x.getNode(), nameschar, NULL, DDDMP_MODE_TEXT, DDDMP_VARNAMES, &outfile[0], NULL);
    
    std::cout << "end" << std::endl;*/
    
    
    //Cudd mgr(0,2);
    // mgr.makeVerbose();		// trace constructors and destructors
    //testBdd(mgr,verbosity);
    //testBdd2(mgr,verbosity);
    //testBdd3(mgr,verbosity);
    //testBdd4(mgr,verbosity);
    //testBdd5(mgr,verbosity);
    //if (verbosity) mgr.info();
    return 0;

} // main


/**Function********************************************************************

  Synopsis    [Test basic operators on BDDs.]

  Description [Test basic operators on BDDs. The function returns void
  because it relies on the error handling done by the interface. The
  default error handler causes program termination.]

  SideEffects [Creates BDD variables in the manager.]

  SeeAlso     [testBdd2 testBdd3 testBdd4 testBdd5]

******************************************************************************/
static void
testBdd(
  Cudd& mgr,
  int verbosity)
{
    if (verbosity) cout << "Entering testBdd\n";
    // Create two new variables in the manager. If testBdd is called before
    // any variable is created in mgr, then x gets index 0 and y gets index 1.
    BDD x = mgr.bddVar();
    BDD y = mgr.bddVar();

    BDD f = x * y;
    if (verbosity) cout << "f"; f.print(2,verbosity);

    BDD g = y + !x;
    if (verbosity) cout << "g"; g.print(2,verbosity);

    if (verbosity) 
        cout << "f and g are" << (f == !g ? "" : " not") << " complementary\n";
    if (verbosity) 
        cout << "f is" << (f <= g ? "" : " not") << " less than or equal to g\n";

    g = f | ~g;
    if (verbosity) cout << "g"; g.print(2,verbosity);

    BDD h = f = y;
    if (verbosity) cout << "h"; h.print(2,verbosity);

    if (verbosity) cout << "x + h has " << (x+h).nodeCount() << " nodes\n";

    h += x;
    if (verbosity) cout << "h"; h.print(2,verbosity);

} // testBdd


/**Function********************************************************************

  Synopsis    [Test vector operators on BDDs.]

  Description [Test vector operators on BDDs. The function returns void
  because it relies on the error handling done by the interface. The
  default error handler causes program termination.]

  SideEffects [May create BDD variables in the manager.]

  SeeAlso     [testBdd testBdd3 testBdd4 testBdd5]

******************************************************************************/
static void
testBdd2(
  Cudd& mgr,
  int verbosity)
{
    if (verbosity) cout << "Entering testBdd2\n";
    vector<BDD> x(4);
    for (int i = 0; i < 4; i++) {
	x[i] = mgr.bddVar(i);
    }

    // Create the BDD for the Achilles' Heel function.
    BDD p1 = x[0] * x[2];
    BDD p2 = x[1] * x[3];
    BDD f = p1 + p2;
    const char* inames[] = {"x0", "x1", "x2", "x3"};
    if (verbosity) {
        cout << "f"; f.print(4,verbosity);
        cout << "Irredundant cover of f:" << endl; f.PrintCover();
        cout << "Number of minterms (arbitrary precision): "; f.ApaPrintMinterm(4);
        cout << "Number of minterms (extended precision):  "; f.EpdPrintMinterm(4);
        cout << "Two-literal clauses of f:" << endl;
        f.PrintTwoLiteralClauses((char **)inames); cout << endl;
    }

    vector<BDD> vect = f.CharToVect();
    if (verbosity) {
        for (size_t i = 0; i < vect.size(); i++) {
            cout << "vect[" << i << "]" << endl; vect[i].PrintCover();
        }
    }

    // v0,...,v3 suffice if testBdd2 is called before testBdd3.
    if (verbosity) {
        const char* onames[] = {"v0", "v1", "v2", "v3", "v4", "v5"};
        mgr.DumpDot(vect, (char **)inames,(char **)onames);
    }

} // testBdd2


/**Function********************************************************************

  Synopsis    [Test additional operators on BDDs.]

  Description [Test additional operators on BDDs. The function returns
  void because it relies on the error handling done by the
  interface. The default error handler causes program termination.]

  SideEffects [May create BDD variables in the manager.]

  SeeAlso     [testBdd testBdd2 testBdd4 testBdd5]

******************************************************************************/
static void
testBdd3(
  Cudd& mgr,
  int verbosity)
{
    if (verbosity) cout << "Entering testBdd3\n";
    vector<BDD> x(6);
    for (int i = 0; i < 6; i++) {
	x[i] = mgr.bddVar(i);
    }

    BDD G = x[4] + !x[5];
    BDD H = x[4] * x[5];
    BDD E = x[3].Ite(G,!x[5]);
    BDD F = x[3] + !H;
    BDD D = x[2].Ite(F,!H);
    BDD C = x[2].Ite(E,!F);
    BDD B = x[1].Ite(C,!F);
    BDD A = x[0].Ite(B,!D);
    BDD f = !A;
    if (verbosity) cout << "f"; f.print(6,verbosity);

    BDD f1 = f.RemapUnderApprox(6);
    if (verbosity) cout << "f1"; f1.print(6,verbosity);
    if (verbosity) 
        cout << "f1 is" << (f1 <= f ? "" : " not") << " less than or equal to f\n";

    BDD g;
    BDD h;
    f.GenConjDecomp(&g,&h);
    if (verbosity) {
        cout << "g"; g.print(6,verbosity);
        cout << "h"; h.print(6,verbosity);
        cout << "g * h " << (g * h == f ? "==" : "!=") << " f\n";
    }

} // testBdd3

/**Function********************************************************************

  Synopsis    [Test transfer between BDD managers.]

  Description [Test transfer between BDD managers.  The
  function returns void because it relies on the error handling done by
  the interface.  The default error handler causes program
  termination.]

  SideEffects [May create BDD variables in the manager.]

  SeeAlso     [testBdd testBdd2 testBdd3 testBdd5]

******************************************************************************/
static void
testBdd4(
  Cudd& mgr,
  int verbosity)
{
    if (verbosity) cout << "Entering testBdd4\n";
    BDD x = mgr.bddVar(0);
    BDD y = mgr.bddVar(1);
    BDD z = mgr.bddVar(2);

    BDD f = !x * !y * !z + x * y;
    if (verbosity) cout << "f"; f.print(3,verbosity);

    Cudd otherMgr(0,0);
    BDD g = f.Transfer(otherMgr);
    if (verbosity) cout << "g"; g.print(3,verbosity);

    BDD h = g.Transfer(mgr);
    if (verbosity) 
        cout << "f and h are" << (f == h ? "" : " not") << " identical\n";

} // testBdd4



/**Function********************************************************************

  Synopsis    [Test maximal expansion of cubes.]

  Description [Test maximal expansion of cubes.  The function returns
  void because it relies on the error handling done by the interface.
  The default error handler causes program termination.]

  SideEffects [May create BDD variables in the manager.]

  SeeAlso     [testBdd testBdd2 testBdd3 testBdd4]

******************************************************************************/
static void
testBdd5(
  Cudd& mgr,
  int verbosity)
{
    if (verbosity) cout << "Entering testBdd5\n";
    vector<BDD> x;
    x.reserve(4);
    for (int i = 0; i < 4; i++) {
	x.push_back(mgr.bddVar(i));
    }
    const char* inames[] = {"a", "b", "c", "d"};
    BDD f = (x[1] & x[3]) | (x[0] & !x[2] & x[3]) | (!x[0] & x[1] & !x[2]);
    BDD lb = x[1] & !x[2] & x[3];
    BDD ub = x[3];
    BDD primes = lb.MaximallyExpand(ub,f);
    assert(primes == (x[1] & x[3]));
    BDD lprime = primes.LargestPrimeUnate(lb);
    assert(lprime == primes);
    if (verbosity) {
      const char * onames[] = {"lb", "ub", "f", "primes", "lprime"};
        vector<BDD> z;
        z.reserve(5);
        z.push_back(lb);
        z.push_back(ub);
        z.push_back(f);
        z.push_back(primes);
        z.push_back(lprime);
        mgr.DumpBlif(z, (char **)inames, (char **)onames);
        cout << "primes(1)"; primes.print(4,verbosity);
    }

    lb = !x[0] & x[2] & x[3];
    primes = lb.MaximallyExpand(ub,f);
    assert(primes == mgr.bddZero());
    if (verbosity) {
        cout << "primes(2)"; primes.print(4,verbosity);
    }

    lb = x[0] & !x[2] & x[3];
    primes = lb.MaximallyExpand(ub,f);
    assert(primes == lb);
    lprime = primes.LargestPrimeUnate(lb);
    assert(lprime == primes);
    if (verbosity) {
        cout << "primes(3)"; primes.print(4,verbosity);
    }

    lb = !x[0] & x[1] & !x[2] & x[3];
    ub = mgr.bddOne();
    primes = lb.MaximallyExpand(ub,f);
    assert(primes == ((x[1] & x[3]) | (!x[0] & x[1] & !x[2])));
    lprime = primes.LargestPrimeUnate(lb);
    assert(lprime == (x[1] & x[3]));
    if (verbosity) {
        cout << "primes(4)"; primes.print(4,1); primes.PrintCover();
    }

    ub = !x[0] & x[3];
    primes = lb.MaximallyExpand(ub,f);
    assert(primes == (!x[0] & x[1] & x[3]));
    lprime = primes.LargestPrimeUnate(lb);
    assert(lprime == primes);
    if (verbosity) {
        cout << "primes(5)"; primes.print(4,verbosity);
    }

} // testBdd5
