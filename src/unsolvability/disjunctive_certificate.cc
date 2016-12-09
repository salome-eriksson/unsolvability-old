#include "disjunctive_certificate.h"

#include "dddmp.h"
#include "cudd.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cassert>
#include <stdlib.h>
#include <sstream>

DisjunctiveCertificate::DisjunctiveCertificate(Task *task, std::ifstream &stream, int r_)
    :  Certificate(task), r(r_) {

    //oftenly used variables
    std::string line;
    std::vector<std::string> line_arr;

    std::getline(stream, line);
    split(line, line_arr, ':');
    if(line_arr[0].compare("bdd-files") != 0) {
        print_parsing_error_and_exit(line, "bdd-files:<#bdd-files>");
    }
    assert(line_arr.size() == 2);
    int amount = stoi(line_arr[1]);
    for(int i = 0; i < amount; ++i) {
        std::string certificate_file;
        std::getline(stream, certificate_file);
        parse_bdd_file(certificate_file);
    }

    if(std::getline(stream, line)) {
        line_arr.clear();
        split(line, line_arr, ':');
        if(line_arr[0].compare("hints") != 0) {
            print_parsing_error_and_exit(line, "hints:<hints file>");
        }
        assert(line_arr.size() == 2);
        std::string hint_file = line_arr[1];
        std::cout << "Hint file: " << hint_file << std::endl;
        hint_stream.open(hint_file);
    }

}

bool DisjunctiveCertificate::contains_state(const Cube &state) {
    BDD statebdd = build_bdd_from_cube(state);

    for(CertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        if(statebdd.Leq(it->second.bdd)) {
            return true;
        }
    }
    return false;
}

bool DisjunctiveCertificate::contains_goal() {
    BDD goalbdd = build_bdd_from_cube(task->get_goal());

    for(CertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        // here we cannot use subset, since we also need to return true if the certificate
        // contains only some but not all goal states
        goalbdd = goalbdd.Intersect(it->second.bdd);
        if(!goalbdd.IsZero()) {
            return true;
        }
    }
    return false;
}

bool DisjunctiveCertificate::check_hints(std::vector<BDD> &action_bdds) {
    // permutation for renaming the certificate to the primed variables
    int permutation[task->get_number_of_facts()*2];
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }

    // oftenly used variables
    std::string line;
    int hint_amount = -1;
    int tmp = -1;
    int index = -1;
    std::vector<int> hints = std::vector<int>(task->get_number_of_actions(), -1);

    std::getline(hint_stream, line);
    while(line.compare("end hints") != 0) {
        // read index and hints
        std::stringstream ss(line);
        ss >> index;
        assert(index >= 0);
        ss >> hint_amount;
        assert(hint_amount >= 0);
        for(int i = 0; i < hint_amount; ++i) {
            ss >> tmp;
            assert(tmp >=0);
            ss >> hints[tmp];
            assert(hints[tmp] >= 0);
            tmp = -1;
        }
        hint_amount = -1;

        // check inductivity for the bdd with given index
        BDD cert_i = certificate[index].bdd;
        BDD cert_i_perm = cert_i.Permute(permutation);
        // loop over actions
        for(size_t i = 0; i < action_bdds.size(); ++i) {
            BDD succ = cert_i * action_bdds[i];
            if(succ.IsZero()) {
                continue;
            }
            // if a hint is given, the sucessors must be included in the bdd with
            // index given by then hint
            if(hints[i] >= 0) {
                BDD tmp = certificate[index].bdd;
                tmp.Permute(permutation);
                if(!succ.Leq(tmp)) {
                    return false;
                }
                // reset hint vector
                hints[i] = -1;
            // if no hint is given, the bdd must be self-inductive in this action
            } else {
                if(!succ.Leq(cert_i_perm)) {
                    return false;
                }
            }
        }
        // set bdd as covered
        certificate[index].covered = true;
        index = -1;

        // read in next line
        std::getline(hint_stream, line);
    }
    hint_stream.close();
    return true;
}

bool DisjunctiveCertificate::is_inductive() {
    // build action bdds
    std::vector<BDD> action_bdds(task->get_number_of_actions(), BDD());
    for(size_t i = 0; i < action_bdds.size(); ++i) {
        action_bdds[i] = build_bdd_for_action(task->get_action(i));
    }

    // read in hints and check if the corresponding bdds are inductive
    // if not, we can return false already, if they are we need to check the rest
    if(hint_stream.is_open()) {
        if(!check_hints(action_bdds)) {
            return false;
        }
    }

    // permutation for renaming the certificate to the primed variables
    int permutation[task->get_number_of_facts()*2];
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }

    // check over all bdds that are not covered yet
    for (CertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        // bdd is covered
        if(it->second.covered) {
            continue;
        }

        // bdd is not covered -> loop over all actions and check if its self-inductive
        BDD cert_i = it->second.bdd;
        BDD cert_i_perm = cert_i.Permute(permutation);
        for(size_t i = 0; i < action_bdds.size(); ++i) {
            BDD succ = cert_i * action_bdds[i];
            if(!succ.Leq(cert_i_perm)) {
                return false;
            }
        }
        it->second.covered = true;
    }

    // sanity check
    for (CertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        // bdd is covered
        if(!it->second.covered) {
            std::cout << "Did not check all subcertificates, this should not happen!" << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
    }

    return true;
}
