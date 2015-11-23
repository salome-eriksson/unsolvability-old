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
  BDD build_primed_bdd();
public:
  Certificate(Task *task);
  virtual ~Certificate();
  virtual bool is_certificate_for(const Cube &state);
  virtual bool is_inductive() = 0;
  //The variable ordering of the cube corresponds to the global variable ordering
  //(as defined in Task)
  virtual bool contains_cube(const Cube &cube) = 0;

  virtual void dump_bdd(BDD& bdd, std::string filename);
};
  
#endif /* CERTIFICATE_H */
