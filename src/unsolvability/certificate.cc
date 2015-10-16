#include "certificate.h"
#include <string.h>
#include <cassert>

Certificate::Certificate(Task *task) : task(task) {

}

Certificate::~Certificate() {

}

void Certificate::parse_fact_names_to_bdd_var(std::string facts) {
  int fact_amount_with_primed = task->get_number_of_facts()*2;
  std::cout << "fact amount: " << fact_amount_with_primed << std::endl;
  fact_names = new char*[fact_amount_with_primed];
  std::vector<std::string> facts_vector;
  split(facts, facts_vector, '|');
  atom_to_bddvar = std::vector<int>(task->get_number_of_facts(), -1);
  int count = 0;
  for(size_t i = 0; i < facts_vector.size(); ++i){
    std::string fact = facts_vector[i];
    std::cout << fact << std::endl;
    //map from global fact index to certificate fact index (unprimed and primed)
    int global_index = task->find_fact(fact);
    atom_to_bddvar[global_index] = count;
    
    //fill in fact_names char** (for reading in bdd)
    fact_names[count] = new char[fact.size()+1];
    strcpy(fact_names[count++], fact.c_str());
    //primed fact
    fact = fact + "'";
    fact_names[count] = new char[fact.size()+1];
    strcpy(fact_names[count++], fact.c_str());
  }
  
  //fill in fact names that don't occur in the certificate
  int number_of_facts = task->get_number_of_facts();
  for(size_t i = 0; i < number_of_facts; ++i) {
    if(atom_to_bddvar[i] == -1) {
      std::string fact = task->get_fact(i);
      std::cout << fact << std::endl;
      atom_to_bddvar[i] = count;
      fact_names[count] = new char[fact.size()+1];
      strcpy(fact_names[count++], fact.c_str());
      //primed fact
      fact = fact + "'";
      fact_names[count] = new char[fact.size()+1];
      strcpy(fact_names[count++], fact.c_str());
    }
  }
  assert(count == fact_amount_with_primed);
}

BDD Certificate::build_bdd_for_action(const Action &a) {
    BDD ret = manager.bddOne();
    for(size_t i = 0; i < a.pre.size(); ++i) {
        ret = ret * manager.bddVar(atom_to_bddvar[a.pre[i]]);
    }
    //the +1 below is for getting the primed version of the variable
    for(size_t i = 0; i < a.add.size(); ++i) {
        ret = ret * manager.bddVar(atom_to_bddvar[a.add[i]]+1);
    }
    for(size_t i = 0; i < a.del.size(); ++i) {
        ret = ret - manager.bddVar(atom_to_bddvar[a.del[i]]+1);
    }
    return ret;
}
