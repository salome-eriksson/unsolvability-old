#ifndef CONJUNCTIVECERTIFICATE_H
#define CONJUNCTIVECERTIFICATE_H

#include <iostream>
#include <fstream>

#include "cudd.h"

#include "certificate.h"


class ConjunctiveCertificate : public Certificate
{
private:
   int r;
   std::ifstream hint_stream;
   bool has_hints;
   std::vector<CertMap::iterator> lastits;

   bool check_hints(std::vector<BDD> &action_bdds);

   void initialize_itvec(std::vector<CertMap::iterator>& itvec);
   bool next_permutation(std::vector<CertMap::iterator>& itvec, std::vector<CertMap::iterator>& lastits);

   // checks if any disjunction over r BDDs coveres the successor_bdd
   bool is_covered_by_r(BDD &bdd_primed, BDD &actionbdd);
public:
  ConjunctiveCertificate(Task *task, std::ifstream &stream, int r);
  bool contains_state(const Cube &state);
  // WARNING: this returns true as soon as there is no r-conjunction refuting goal inclusion
  // even if the full conjunction actually does not contain any goal
  bool contains_goal();
  // WARNING: this returns false if it is not r-inductive for the specified r
  bool is_inductive();
};

#endif // CONJUNCTIVECERTIFICATE_H
