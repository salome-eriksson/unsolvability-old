#include "simple_certificate.h"
#include <fstream>
#include <cassert>


SimpleCertificate::SimpleCertificate(Task *task, std::ifstream &in)
    : Certificate(task){

    std::string line;
    std::getline(in, line);
    std::vector<std::string> line_arr;
    split(line, line_arr, ':');
    if(line_arr[0].compare("File") != 0) {
        print_parsing_error_and_exit(line, "File:<filename");
    }
    assert(line_arr.size() == 2);
    std::string certificate_file = line_arr[1];
    std::cout << "simple certificate file: " << certificate_file << std::endl;
    std::getline(in, line);
    if(line.compare("begin_variables") != 0) {
        print_parsing_error_and_exit(line, "begin_variables");
    }
    read_in_variable_order(in);
    std::vector<BDD> bddvec;
    parse_bdd_file(certificate_file, bddvec);
    assert(bddvec.size() == 1);
    certificate = bddvec[0];
    std::getline(in, line);
    if(line.compare("end_certificate") != 0) {
        print_parsing_error_and_exit(line, "end_certificate");
    }
}


// TODO: is there an easier way to check if a cube is contained in a bdd
// (similar to problem in search - easy way to add cube to bdd)
bool SimpleCertificate::contains_state(const Cube &state) {
    BDD statebdd = build_bdd_from_cube(state);

    // if the state is included in the certificate, its bdd must represent a subset
    // of the certificate bdd (since both bdds use only unprimed variables)
    if(!statebdd.Leq(certificate)) {
        return false;
    }
    return true;
}

bool SimpleCertificate::contains_goal() {
    BDD goalbdd = build_bdd_from_cube(task->get_goal());
    // here we cannot use subset, since we also need to return true if the certificate
    // contains only some but not all goal states
    goalbdd = goalbdd.Intersect(certificate);
    if(goalbdd.IsZero()) {
        return false;
    }
    return true;
}

bool SimpleCertificate::is_inductive() {

    int factamount = task->get_number_of_facts();

    // permutation for renaming the certificate to the primed variables
    int permutation[factamount*2];
    for(int i = 0 ; i < factamount; ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }

    BDD c_primed = certificate.Permute(permutation);

    // loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);
        // succ represents pairs of states (from the certificate)
        // and successors achieved with action a
        BDD succ = action_bdd * certificate;

        // if succ is not a subset of c_primed, it contains successors which are not in
        // c primed --> not inductive
        if(!succ.Leq(c_primed)) {
            return false;
        }
    }
    return true;
}
