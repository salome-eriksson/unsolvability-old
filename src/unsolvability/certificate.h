#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <vector>
#include <string>
#include "task.h"
#include "global_funcs.h"
#include "cuddObj.hh"

class Certificate {
protected:
  std::vector<int> fact_to_bddvar;
  int original_bdd_vars; //amount of original bdd vars (without unused and primed vars)
  Task* task;
  Cudd manager;
  void map_global_facts_to_bdd_var(std::ifstream &facts);
  void parse_bdd_file(std::string bddfile, std::vector<BDD> &bdds);
  BDD build_bdd_for_action(const Action &a);
public:
  Certificate(Task *task);
  virtual ~Certificate();
  virtual bool is_unsolvability_certificate() = 0;
  virtual bool is_inductive() = 0;
  virtual bool contains_state(const State &state) = 0;
  virtual bool contains_goal() = 0;
};
  
#endif /* CERTIFICATE_H */
