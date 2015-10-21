#include "simple_certificate.h"
#include <fstream>

#include "dddmp.h"

SimpleCertificate::SimpleCertificate(Task *task, std::ifstream &in)
    : Certificate(task){

    manager = Cudd(task->get_number_of_facts()*2,0);

    std::string line;
    std::getline(in, line);
    std::vector<std::string> line_arr;
    split(line, line_arr, ':');
    if(line_arr[0].compare("File") != 0) {
        print_parsing_error_and_exit(line, "File:<filename");
    }
    assert(line_arr.size() == 2);
    std::string certificate_file = line_arr[1];
    std::getline(in, line);
    if(line.compare("begin_variables") != 0) {
        print_parsing_error_and_exit(line, "begin_variables");
    }
    map_global_facts_to_bdd_var(in);
    std::vector<BDD> bddvec;
    parse_bdd_file(certificate_file, bddvec);
    assert(bddvec.size() == 1);
    certificate = bddvec[0];


    //TEST
    /*int n = task->get_number_of_facts();
    std::vector<std::string> names(n*2);
    for(size_t i = 0; i < n; ++i) {
        names[fact_to_bddvar[i]] = task->get_fact(i);
        names[fact_to_bddvar[i]+1] = task->get_fact(i) + "'";
    }

    char **nameschar = new char*[names.size()];
    for(size_t i = 0; i < names.size(); i++){
        nameschar[i] = new char[names[i].size() + 1];
        strcpy(nameschar[i], names[i].c_str());
    }
    std::string filename = "testbdd.txt";
    std::string bddname = "parsed_bdd";
    FILE* f = fopen(filename.c_str(), "w");
    Dddmp_cuddBddStore(manager.getManager(),&bddname[0], certificate.getNode(), nameschar, NULL,
            DDDMP_MODE_TEXT, DDDMP_VARNAMES, &filename[0], f);
    fclose(f);

    std::cout << "var order:" << std::endl;
    for(size_t i = 0; i < fact_to_bddvar.size(); ++i) {
        assert(fact_to_bddvar[i] % 2 == 0);
        std::cout << i << " " << fact_to_bddvar[i] << std::endl;
    }*/
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
        statebdd = statebdd * manager.bddVar(fact_to_bddvar[i]);
      } else {
        statebdd = statebdd - manager.bddVar(fact_to_bddvar[i]);
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
      goalbdd = goalbdd * manager.bddVar(fact_to_bddvar[goal[i]]);
    }
    BDD result = certificate * goalbdd;
    if (result.IsZero()) {
      return false;
    }
    return true;
}
