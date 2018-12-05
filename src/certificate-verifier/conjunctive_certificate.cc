#include "conjunctive_certificate.h"

#include "dddmp.h"
#include "cudd.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cassert>
#include <stdlib.h>
#include <sstream>

ConjunctiveCertificate::ConjunctiveCertificate(Task *task, std::ifstream &stream, int r_)
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

    //initialize lastits (stores the last r map iterators)
    lastits.resize(r);
    CertMap::iterator it = certificate.begin();
    for(int i = 0; i < certificate.size() - r; ++i) {
        it++;
    }
    for(int i = 0; i < r; ++i) {
        lastits[i] = it++;
    }

}

void ConjunctiveCertificate::initialize_itvec(std::vector<CertMap::iterator> &itvec) {
    CertMap::iterator it = certificate.begin();
    itvec.resize(r, it);
    for(int i = 0; i < r; ++i) {
        itvec[i] = it++;
    }
}

/*
 * This function returns the next ordered permutation of itvec. It returns false if there
 * is no next permutation. One can think of it as "counting up":
 * It checks starting from the end of itvec if the iterator has the highest possible "value"
 * (lastit stores this "value" - for the last element it is the last iterator etc).
 * When it has found a position where this is not the case it increments this iterator by one
 * and sets all subsequent iterator to one higher than the preceeding one.
 * Example with numbers instead of iterators:
 * Input: itvec: 1 5 6 7, lastits: 4 5 6 7 -> itvec changed to 2 3 4 5 and return true
 *
 * sidenote: itvec and lastits must have size r!
 */
bool ConjunctiveCertificate::next_permutation(std::vector<CertMap::iterator>& itvec, std::vector<CertMap::iterator>& lastits) {
    int pos = r-1;
    while(itvec[pos] == lastits[pos]) {
        pos--;
        }
    if(pos < 0) {
        return false;
    } else {
        itvec[pos]++;
        for(pos = pos+1; pos < r; pos++) {
            itvec[pos] = itvec[pos-1];
            itvec[pos]++;
        }
    }
    return true;
}

bool ConjunctiveCertificate::is_covered_by_r(BDD &bdd_primed, BDD &actionbdd) {
    std::vector<CertMap::iterator> itvec;
    initialize_itvec(itvec);
    do {
        BDD conjunction = manager.bddOne();
        for(int i = 0; i < itvec.size(); ++i) {
            conjunction = conjunction * itvec[i]->second.bdd;
        }
        conjunction = conjunction * actionbdd;
        if(conjunction.Leq(bdd_primed)) {
            return true;
        }
    } while(next_permutation(itvec, lastits));
    return false;
}

bool ConjunctiveCertificate::contains_state(const Cube &state) {
    BDD statebdd = build_bdd_from_cube(state);

    for(CertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        if(!statebdd.Leq(it->second.bdd)) {
            return false;
        }
    }
    return true;
}

bool ConjunctiveCertificate::contains_goal() {
    BDD goalbdd = build_bdd_from_cube(task->get_goal());
    BDD notgoal = !goalbdd;

    //TODO: figure out how we can add hint for this!
    std::vector<CertMap::iterator> itvec;
    initialize_itvec(itvec);

    do {
        BDD conjunction = manager.bddOne();
        for(int i = 0; i < itvec.size(); ++i) {
            conjunction = conjunction * itvec[i]->second.bdd;
        }
        if (conjunction.Leq(notgoal)) {
            return false;
        }
    } while(next_permutation(itvec, lastits));

    return true;
}

bool ConjunctiveCertificate::check_hints(std::vector<BDD> &action_bdds) {
    // oftenly used variables
    std::string line;
    int hint_amount = -1;
    int action = -1;
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
            ss >> action;
            assert(action >=0);
            ss >> hints[action];
            assert(hints[action] >= 0);
            action = -1;
        }
        hint_amount = -1;

        // check inductivity for the bdd with given index
        BDD cert_i_perm = certificate[index].bdd.Permute(&permutation[0]);
        for(size_t i = 0; i < action_bdds.size(); ++i) {
            // if a hint is given, the sucessor of the hint must be included by bdd[index]
            if(hints[i] >= 0) {
                BDD tmp = certificate[hints[i]].bdd;
                tmp = tmp * action_bdds[i];
                if(!tmp.Leq(cert_i_perm)) {
                    return false;
                }
                // reset hint vector
                hints[i] = -1;
            // if no hint is given, check if the bdd is r-inductive
            // if not, the certificate is not valid
            } else if (!(is_covered_by_r(cert_i_perm, action_bdds[i]))) {
                return false;
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

bool ConjunctiveCertificate::is_inductive() {
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

    // check over all bdds that are not covered yet
    for (CertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        // bdd is covered
        if(it->second.covered) {
            continue;
        }

        // bdd is not covered -> loop over all actions and check if its r-inductive
        BDD cert_i_perm = it->second.bdd.Permute(&permutation[0]);
        for(size_t i = 0; i < action_bdds.size(); ++i) {
            if(!(is_covered_by_r(cert_i_perm, action_bdds[i]))) {
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
