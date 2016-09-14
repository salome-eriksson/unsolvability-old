#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <vector>
#include <string>
#include "task.h"
#include "global_funcs.h"
#include "cuddObj.hh"


/*
 * Each certificate has its own variable order. Fact_to_bddvar is used to map
 * from the "global" variable order (given by the task) to the local one.
 * Note that each certificate contains ALL variables from the task (even those which
 * were not present when creating the certificate), and they also have a primed version
 * of each variable right next to the unprimed one.
 */

class Certificate {
protected:
  std::vector<int> fact_to_bddvar;
  Task* task;
  Cudd manager;
  // also builds the mapping between global facts and bdd variables
  void read_in_variable_order(std::ifstream &facts);
  void parse_bdd_file(std::string bddfile, std::vector<BDD> &bdds);
  BDD build_bdd_for_action(const Action &a);
  BDD build_bdd_from_cube(const Cube &cube);
public:
  Certificate(Task *task);
  virtual ~Certificate();
  // The variable ordering of the cube corresponds to the global variable ordering
  // (as defined in Task)
  virtual bool contains_state(const Cube &state) = 0;
  virtual bool contains_goal() = 0;
  virtual bool is_inductive() = 0;
  virtual bool is_certificate_for(const Cube &state);

  virtual void dump_bdd(BDD& bdd, std::string filename);
};
  
#endif /* CERTIFICATE_H */
