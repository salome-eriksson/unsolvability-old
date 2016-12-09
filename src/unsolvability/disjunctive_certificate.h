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

   void store_state_bdds(std::string file);
   void store_hcert_bdds(std::string file);
   bool check_hints(std::vector<BDD> &action_bdds);
public:
  DisjunctiveCertificate(Task *task, std::ifstream &stream, int r);
  bool contains_state(const Cube &state);
  bool contains_goal();
  bool is_inductive();
};

#endif // DISJUNCTIVECERTIFICATE_H
