#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <vector>
#include <string>
#include <unordered_map>

#include "task.h"
#include "global_funcs.h"

#include "cuddObj.hh"

struct CertEntry {
    BDD bdd;
    bool covered;
    CertEntry() : bdd(BDD()), covered(false) {}
    CertEntry(BDD _bdd, bool _covered) : bdd(_bdd), covered(_covered) {}
};

struct BDDAction {
    BDD pre;
    BDD eff;
    BDDAction(BDD pre, BDD eff) : pre(pre), eff(eff) {}
};

typedef std::unordered_map<int, CertEntry> CertMap;

class Certificate {
protected:
  Task* task;
  Cudd manager;
  CertMap certificate;
  // array needed for permuting bdds to primed version
  std::vector<int> permutation;
  void parse_bdd_file(std::string bddfile);
  std::vector<BDDAction> bdd_actions;
  BDD build_bdd_from_cube(const Cube &cube);
public:
  Certificate(Task *task);
  virtual ~Certificate();
  virtual bool contains_state(const Cube &state) = 0;
  virtual bool contains_goal() = 0;
  virtual bool is_inductive() = 0;
  virtual bool is_certificate_for(const Cube &state);

  virtual void dump_bdd(BDD& bdd, std::string filename);
};
  
#endif /* CERTIFICATE_H */
