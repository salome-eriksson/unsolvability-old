#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <vector>
#include <string>
#include "task.h"
#include "global_funcs.h"
#include "cuddObj.hh"

class Certificate {
protected:
  std::vector<int> atom_to_bddvar;
  char** fact_names; //needed for reading in the BDD(s)
  Task* task;
  Cudd manager;
  void parse_fact_names_to_bdd_var(std::string facts);
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
