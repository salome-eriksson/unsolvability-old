#ifndef DISJUNCTIVECERTIFICATE_H
#define DISJUNCTIVECERTIFICATE_H

#include <iostream>
#include <fstream>

#include "cudd.h"

#include "certificate.h"


class DisjunctiveCertificate : public Certificate {
private:
   int r;
   std::ifstream hint_stream;
   bool has_hints;
   std::vector<CertMap::iterator> lastits;

   bool check_hints(std::vector<BDD> &action_bdds);

   void initialize_itvec(std::vector<CertMap::iterator>& itvec);
   bool next_permutation(std::vector<CertMap::iterator>& itvec, std::vector<CertMap::iterator>& lastits);

   // checks if any disjunction over r BDDs coveres the successor_bdd
   bool is_covered_by_r(BDD &successor_bdd);
public:
  DisjunctiveCertificate(Task *task, std::ifstream &stream, int r);
  bool contains_state(const Cube &state);
  bool contains_goal();
  // WARNING: this returns false if it is not r-inductive for the specified r
  bool is_inductive();
};

#endif // DISJUNCTIVECERTIFICATE_H
