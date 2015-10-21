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
}

bool SimpleCertificate::is_inductive() {
    //loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);

        int factamount = task->get_number_of_facts();

        //permutation for renaming the certificate to the primed variables
        int permutation[factamount*2];
        for(int i = 0 ; i < factamount; ++i) {
          permutation[2*i] = (2*i)+1;
          permutation[(2*i)+1] = 2*i;
        }
        BDD c_primed = certificate.Permute(permutation);
        // succ represents pairs of states and successors with action a
        BDD succ = action_bdd * certificate;
        // res contains pairs of states from above where the successor is not in c (primed)
        BDD res = succ - c_primed;

        if(!res.IsZero()) {
            return false;
        }
    }
    return true;
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
