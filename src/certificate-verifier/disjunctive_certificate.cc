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

void DisjunctiveCertificate::initialize_itvec(std::vector<CertMap::iterator> &itvec) {
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
bool DisjunctiveCertificate::next_permutation(std::vector<CertMap::iterator>& itvec, std::vector<CertMap::iterator>& lastits) {
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

bool DisjunctiveCertificate::is_covered_by_r(BDD &successor_bdd) {
    std::vector<CertMap::iterator> itvec;
    initialize_itvec(itvec);
    int count = 0;
    do {
        count++;
        BDD disjunction = manager.bddZero();
        for(int i = 0; i < itvec.size(); ++i) {
            disjunction = disjunction + itvec[i]->second.bdd;
        }
        disjunction.Permute(&permutation[0]);
        if(successor_bdd.Leq(disjunction)) {
            return true;
        }
    } while(next_permutation(itvec, lastits));
    return false;
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
    BDD notgoal = !goalbdd;

    for(CertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        //each part of the union must contain no goal states
        if(!it->second.bdd.Leq(notgoal)) {
            return true;
        }
    }
    return false;
}

bool DisjunctiveCertificate::check_hints() {
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
        BDD cert_i = certificate[index].bdd;
        // loop over actions
        for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
            const Action &action = task->get_action(i);
            BDD succ = cert_i * bdd_actions[i].pre;
            for (int var = 0; var < task->get_number_of_facts(); ++var) {
                if (action.change[var] != 0) {
                    permutation[2*var] = 2*var+1;
                    permutation[2*var+1] = 2*var;
                } else {
                    permutation[2*var] = 2*var;
                    permutation[2*var+1] = 2*var+1;
                }
            }
            succ = succ.Permute(&permutation[0])*bdd_actions[i].eff;
            // if a hint is given, the sucessors must be included in the bdd with
            // index given by then hint
            if(hints[i] >= 0) {
                if(!succ.Leq(certificate[hints[i]].bdd)) {
                    return false;
                }
                // reset hint vector
                hints[i] = -1;
            // if no hint is given, check if the bdd is self- or r-inductive
            // if not, the certificate is not valid
            // TODO: ask if it is guaranteed that the first part will get evaluated first!
            } else if (!(succ.Leq(cert_i)) && !(is_covered_by_r(succ))) {
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

bool DisjunctiveCertificate::is_inductive() {
    // read in hints and check if the corresponding bdds are inductive
    // if not, we can return false already, if they are we need to check the rest
    if(hint_stream.is_open()) {
        if(!check_hints()) {
            return false;
        }
    }

    // check over all bdds that are not covered yet
    for (CertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        // bdd is covered
        if(it->second.covered) {
            continue;
        }

        // bdd is not covered -> loop over all actions and check if its self- or r-inductive
        BDD cert_i = it->second.bdd;
        for(size_t i = 0; i < task->get_number_of_actions(); ++i) {
            const Action &action = task->get_action(i);
            BDD succ = cert_i * bdd_actions[i].pre;
            for (int var = 0; var < task->get_number_of_facts(); ++var) {
                if (action.change[var] != 0) {
                    permutation[2*var] = 2*var+1;
                    permutation[2*var+1] = 2*var;
                } else {
                    permutation[2*var] = 2*var;
                    permutation[2*var+1] = 2*var+1;
                }
            }
            succ = succ.Permute(&permutation[0])*bdd_actions[i].eff;
            if(!(succ.Leq(cert_i)) && !(is_covered_by_r(succ))) {
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
