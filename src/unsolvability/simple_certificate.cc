#include "simple_certificate.h"
#include <fstream>
#include <cassert>


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
    std::cout << "simple certificate file: " << certificate_file;
    std::getline(in, line);
    if(line.compare("begin_variables") != 0) {
        print_parsing_error_and_exit(line, "begin_variables");
    }
    std::cout << "reading in variable order for simple certificate" << std::endl;
    map_global_facts_to_bdd_var(in);
    std::vector<BDD> bddvec;
    std::cout << "parsing bdds in simple certificate file" << std::endl;
    parse_bdd_file(certificate_file, bddvec);
    assert(bddvec.size() == 1);
    certificate = bddvec[0];
    std::getline(in, line);
    if(line.compare("end_certificate") != 0) {
        print_parsing_error_and_exit(line, "end_certificate");
    }
    std::cout << "done building simple certificate" << std::endl;
}

bool SimpleCertificate::is_inductive() {

    int factamount = task->get_number_of_facts();

    //permutation for renaming the certificate to the primed variables
    int permutation[factamount*2];
    for(int i = 0 ; i < factamount; ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }

    BDD c_primed = certificate.Permute(permutation);

    //loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);
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

bool SimpleCertificate::contains_cube(const Cube &cube) {
    BDD statebdd = manager.bddOne();
    for(size_t i = 0; i < cube.size(); ++i) {
      if(cube[i] == 1) {
        statebdd = statebdd * manager.bddVar(fact_to_bddvar[i]);
      } else if(cube[i] == 0){
        statebdd = statebdd - manager.bddVar(fact_to_bddvar[i]);
      } else {
          assert(cube[i] == 2);
      }
    }
    BDD result = certificate * statebdd;
    if(result.IsZero()) {
      return false;
    }
    return true;
}
