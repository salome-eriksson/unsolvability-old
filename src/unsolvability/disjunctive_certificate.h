#ifndef DISJUNCTIVECERTIFICATE_H
#define DISJUNCTIVECERTIFICATE_H

#include <iostream>
#include <unordered_map>
#include <fstream>

#include "cudd.h"

#include "certificate.h"

struct DisjCertEntry {
    BDD bdd;
    bool covered;
    DisjCertEntry() : bdd(BDD()), covered(false) {}
    DisjCertEntry(BDD _bdd, bool _covered) : bdd(_bdd), covered(_covered) {}
};

typedef std::unordered_map<int, DisjCertEntry> DisjCertMap;

class DisjunctiveCertificate : public Certificate {
private:
   DisjCertMap certificate;
   std::ifstream &cert_stream;

   void store_bdds(std::string file);
public:
  DisjunctiveCertificate(Task *task, std::ifstream &stream);
  bool contains_state(const Cube &state);
  bool contains_goal();
  bool is_inductive();
};

#endif // DISJUNCTIVECERTIFICATE_H
