#include "strong_conjunctive_certificate.h"
#include <fstream>
#include <cassert>

StrongConjunctiveCertificate::StrongConjunctiveCertificate(Task *task, std::ifstream &in)
    : Certificate(task) {

    std::string line;
    std::getline(in, line);
    std::vector<std::string> line_arr;
    split(line, line_arr, ':');
    if(line_arr[0].compare("File") != 0) {
        print_parsing_error_and_exit(line, "File:<filename");
    }
    assert(line_arr.size() == 2);
    std::string certificate_file = line_arr[1];
    std::cout << "strong conjunctive certificate file: " << certificate_file << std::endl;
    assert(certificates.empty());
    parse_bdd_file(certificate_file, certificates);
    std::getline(in, line);
    if(line.compare("end_certificate") != 0) {
        print_parsing_error_and_exit(line, "end_certificate");
    }
}

bool StrongConjunctiveCertificate::contains_state(const Cube &state) {
    BDD statebdd = build_bdd_from_cube(state);
    for(size_t i = 0; i < certificates.size(); ++i) {
        if(!statebdd.Leq(certificates[i])) {
            return false;
        }
    }
    return true;
}

bool StrongConjunctiveCertificate::contains_goal() {
    BDD goalbdd = build_bdd_from_cube(task->get_goal());
    for(size_t i = 0; i < certificates.size(); ++i) {
        BDD tmp = goalbdd * certificates[i];
        if(tmp.IsZero()) {
            return false;
        }
    }
    return true;
}

bool StrongConjunctiveCertificate::is_inductive() {
    std::vector<BDD> certificates_primed;

    // permutation for renaming the certificates to the primed variables
    int factamount = task->get_number_of_facts();
    int permutation[factamount*2];
    for(int i = 0 ; i < factamount; ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }
    for(size_t i = 0; i < certificates.size(); ++i) {
        certificates_primed.push_back(certificates[i].Permute(permutation));
    }

    // loop over actions
    for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
        const Action &a = task->get_action(i);
        BDD action_bdd = build_bdd_for_action(a);

        // for each certificate c1 we need to find a certificate c2 such that
        // (c1 \cup c2)[a] \subseteq c1'
        for(size_t ci = 0; ci < certificates.size(); ++ci) {
            bool found_cj = false;
            for(size_t cj = 0; cj < certificates.size(); ++cj) {
                BDD succ = action_bdd * certificates[ci]*certificates[cj];
                if(succ.Leq(certificates_primed[ci])) {
                    found_cj = true;
                    break;
                }
            }// end loop over certificate c2
            if(!found_cj) {
                std::cout << "did not find cj at index " << ci << std::endl;
                return false;
            }
        }// end looping over certificates c1
    }// end loop over actions
    return true;
}
