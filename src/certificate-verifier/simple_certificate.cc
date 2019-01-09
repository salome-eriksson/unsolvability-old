#include "simple_certificate.h"
#include <fstream>
#include <cassert>


SimpleCertificate::SimpleCertificate(Task *task, std::ifstream &in)
    : Certificate(task){

    std::string line;
    std::getline(in, line);
    std::vector<std::string> line_arr;
    split(line, line_arr, ':');
    if(line_arr[0].compare("bdd-files") != 0) {
        print_parsing_error_and_exit(line, "bdd-files:<#bdd-files>");
    }
    assert(line_arr.size() == 2);
    int amount = stoi(line_arr[1]);
    assert(amount == 1);
    std::string certificate_file;
    std::getline(in, certificate_file);
    std::cout << "simple certificate file: " << certificate_file << std::endl;
    parse_bdd_file(certificate_file);
    assert(certificate.size() == 1);
    bdd_certificate = (certificate.begin())->second.bdd;

    if(std::getline(in, line)) {
        print_parsing_error_and_exit(line, "end of file");
    }
}


// TODO: is there an easier way to check if a cube is contained in a bdd
// (similar to problem in search - easy way to add cube to bdd)
bool SimpleCertificate::contains_state(const Cube &state) {
    BDD statebdd = build_bdd_from_cube(state);

    // if the state is included in the certificate, its bdd must represent a subset
    // of the certificate bdd (since both bdds use only unprimed variables)
    if(!statebdd.Leq(bdd_certificate)) {
        return false;
    }
    return true;
}

bool SimpleCertificate::contains_goal() {
    BDD goalbdd = build_bdd_from_cube(task->get_goal());
    // here we cannot use subset, since we also need to return true if the certificate
    // contains only some but not all goal states
    goalbdd = goalbdd.Intersect(bdd_certificate);
    if(goalbdd.IsZero()) {
        return false;
    }
    return true;
}

bool SimpleCertificate::is_inductive() {
    // loop over all actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &action = task->get_action(i);
        BDD res = bdd_certificate*bdd_actions[i].pre;
        for (int var = 0; var < task->get_number_of_facts(); ++var) {
            if (action.change[var] != 0) {
                permutation[2*var] = 2*var+1;
                permutation[2*var+1] = 2*var;
            } else {
                permutation[2*var] = 2*var;
                permutation[2*var+1] = 2*var+1;
            }
        }
        res = res.Permute(&permutation[0])*bdd_actions[i].eff;
        if (!(res.Leq(bdd_certificate))) {
            return false;
        }
    }
    return true;
}
