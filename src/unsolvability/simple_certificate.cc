#include "simple_certificate.h"

#include "dddmp.h"

SimpleCertificate::SimpleCertificate(Task *task, std::vector<std::string> certificate_info)
    : Certificate(task){
  
  //task = task;
  //we need 2 times as much variables as there are facts since we need the primed version as well
  manager = Cudd(task->get_number_of_facts()*2,0);

  assert(certificate_info.size() == 2);
  std::vector<std::string> info_line;
  split(certificate_info[0], info_line, ':');
  if(info_line[0].compare("File") != 0) {
      print_parsing_error_and_exit(certificate_info[0], "File:<filename>");
  }
  std::string bddfile = info_line[1];
  split(certificate_info[1], info_line, ':');
  if(info_line[0].compare("Variables") !=0) {
      print_parsing_error_and_exit(certificate_info[1], "Variables:<vars>");
  }
  std::string fact_names_string = info_line[1];

  //map between global variable order and bdd variable order
  parse_fact_names_to_bdd_var(fact_names_string);
  std::cout << bddfile << std::endl;
  certificate = BDD(manager, Dddmp_cuddBddLoad(manager.getManager(), 
      DDDMP_VAR_MATCHNAMES, fact_names, NULL, NULL, DDDMP_MODE_TEXT, &bddfile[0], NULL));

  //TODO: take away
  std::string filename = "testbdd.txt";
  std::string bddname = "parsed_bdd";
  FILE* f = fopen(filename.c_str(), "w");
  Dddmp_cuddBddStore(manager.getManager(),&bddname[0], certificate.getNode(), fact_names, NULL,
          DDDMP_MODE_TEXT, DDDMP_VARNAMES, &filename[0], f);
  fclose(f);

  std::cout << "var order:";
  for(size_t i = 0; i < atom_to_bddvar.size(); ++i) {
      assert(atom_to_bddvar[i] % 2 == 0);
      std::cout << i+1 << " " << atom_to_bddvar[i]/2+1 << std::endl;
  }
}

bool SimpleCertificate::is_unsolvability_certificate() {
  return contains_state(task->get_initial_state()) && !contains_goal() && is_inductive();
}

bool SimpleCertificate::is_inductive() {
  //loop over all actions
  for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
    const Action &a = task->get_action(i);
    BDD action_bdd = build_bdd_for_action(a);
    
    //permutation for renaming the certificate to the primed variables
    int permutation[task->get_number_of_facts()*2];
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      permutation[i] = i+1;
      permutation[i+1] = i;
    }
    BDD not_c_primed = (!certificate).Permute(permutation);
    
    return (action_bdd * certificate * not_c_primed).IsZero();
  }
}

bool SimpleCertificate::contains_state(const State &state) {
  BDD statebdd = manager.bddOne();
  for(size_t i = 0; i < state.size(); ++i) {
    if(state[i]) {
      statebdd = statebdd * manager.bddVar(atom_to_bddvar[i]);
    } else {
      statebdd = statebdd - manager.bddVar(atom_to_bddvar[i]);
    }
  }
  BDD result = certificate * statebdd;
  if(result.IsZero()) {
    return false;
  }
  return true;
}

bool SimpleCertificate::contains_goal() {
  BDD goalbdd = manager.bddOne();
  const std::vector<int> &goal = task->get_goal();
  for(size_t i = 0; i < goal.size(); ++i) {
    goalbdd = goalbdd * manager.bddVar(atom_to_bddvar[goal[i]]);
  }
  BDD result = certificate * goalbdd;
  if (result.IsZero()) {
    return false;
  }
  return true;
}
