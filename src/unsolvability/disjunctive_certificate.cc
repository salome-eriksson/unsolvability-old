#include "disjunctive_certificate.h"

#include "dddmp.h"
#include "cudd.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cassert>
#include <stdlib.h>
#include <sstream>

DisjunctiveCertificate::DisjunctiveCertificate(Task *task, std::ifstream &stream)
    :  Certificate(task), cert_stream(stream) {

    //oftenly used variables
    std::string line;
    std::vector<std::string> line_arr;

    // parse bdd file
    std::getline(cert_stream, line);
    split(line, line_arr, ':');
    if(line_arr[0].compare("State BDDs") != 0) {
        print_parsing_error_and_exit(line, "State BDDs:<state bdd file>");
    }
    assert(line_arr.size() == 2);
    std::string state_bdds_file = line_arr[1];
    std::cout << "State BDDs file: " << state_bdds_file << std::endl;
    store_state_bdds(state_bdds_file);

    // parse h cert bdd file
    std::getline(cert_stream, line);
    line_arr.clear();
    split(line, line_arr, ':');
    if(line_arr[0].compare("Heuristic Certificates BDDs") != 0) {
        print_parsing_error_and_exit(line, "Heuristic Certificates BDDs:<h_cert bdd file>");
    }
    assert(line_arr.size() == 2);
    std::string hcert_bdds_file = line_arr[1];
    std::cout << "Heuristic Certificates BDDs file: " << hcert_bdds_file << std::endl;
    store_hcert_bdds(hcert_bdds_file);

    std::getline(cert_stream, line);
    if(line.compare("begin hints") != 0) {
        print_parsing_error_and_exit(line, "begin hints");
    }
}

void DisjunctiveCertificate::store_state_bdds(std::string file) {
    int compose[task->get_number_of_facts()];
    for(int i = 0; i < task->get_number_of_facts(); ++i) {
        compose[i] = 2*i;
    }
    int index = -1;
    FILE *fp;
    fp = fopen(file.c_str(), "r");

    while(fscanf(fp, "%d", &index) == 1) {
        assert(index >= 0 && certificate.find(index) == certificate.end());
        DdNode **tmpArray;
        int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
            DDDMP_VAR_COMPOSEIDS,NULL,NULL,&compose[0],DDDMP_MODE_TEXT,NULL,fp,&tmpArray);

        assert(nRoots == 1);
        certificate[index] = DisjCertEntry(BDD(manager, tmpArray[0]), false);
        Cudd_RecursiveDeref(manager.getManager(), tmpArray[0]);
        FREE(tmpArray);
        index = -1;
    }
}

void DisjunctiveCertificate::store_hcert_bdds(std::string file) {
    int compose[task->get_number_of_facts()];
    for(int i = 0; i < task->get_number_of_facts(); ++i) {
        compose[i] = 2*i;
    }
    int amount = -1;
    int res = -1;
    FILE *fp;
    fp = fopen(file.c_str(), "r");
    res = fscanf(fp, "%d", &amount);
    assert(res == 1);
    assert(amount >= 0);
    std::vector<int> indices = std::vector<int>(amount, -1);
    for(int i = 0; i < amount; ++i) {
        res = fscanf(fp, "%d", &indices[i]);
        assert(res == 1);
        assert(indices[i] >= 0);
    }
    DdNode **tmpArray;
    int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
        DDDMP_VAR_COMPOSEIDS,NULL,NULL,&compose[0],DDDMP_MODE_TEXT,NULL,fp,&tmpArray);
    assert(nRoots == amount);

    for (int i=0; i<nRoots; i++) {
        certificate[indices[i]] = DisjCertEntry(BDD(manager, tmpArray[i]), false);
        Cudd_RecursiveDeref(manager.getManager(), tmpArray[i]);
    }
    FREE(tmpArray);

}

// stubs

bool DisjunctiveCertificate::contains_state(const Cube &state) {
    BDD statebdd = build_bdd_from_cube(state);

    for(DisjCertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        if(statebdd.Leq(it->second.bdd)) {
            return true;
        }
    }
    return false;
}

bool DisjunctiveCertificate::contains_goal() {
    BDD goalbdd = build_bdd_from_cube(task->get_goal());

    for(DisjCertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        // here we cannot use subset, since we also need to return true if the certificate
        // contains only some but not all goal states
        goalbdd = goalbdd.Intersect(it->second.bdd);
        if(!goalbdd.IsZero()) {
            return true;
        }
    }
    return false;
}

bool DisjunctiveCertificate::is_inductive() {
    // oftenly used variables
    std::string line;
    int hint_amount = -1;
    int tmp = -1;
    int index = -1;
    std::vector<int> hints = std::vector<int>(task->get_number_of_actions(), -1);

    // build action bdds
    std::vector<BDD> action_bdds(task->get_number_of_actions(), BDD());
    for(size_t i = 0; i < action_bdds.size(); ++i) {
        action_bdds[i] = build_bdd_for_action(task->get_action(i));
    }

    // permutation for renaming the certificate to the primed variables
    int permutation[task->get_number_of_facts()*2];
    for(int i = 0 ; i < task->get_number_of_facts(); ++i) {
      permutation[2*i] = (2*i)+1;
      permutation[(2*i)+1] = 2*i;
    }

    // read in hints and check if the corresponding bdds are inductive
    std::getline(cert_stream, line);
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
        std::getline(cert_stream, line);
    }


    // check over all bdds that are not covered yet - they must be self-inductive
    for (DisjCertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
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
            it->second.covered = true;
        }
    }

    // sanity check
    for (DisjCertMap::iterator it = certificate.begin(); it != certificate.end(); ++it) {
        // bdd is covered
        if(!it->second.covered) {
            std::cout << "Did not check all subcertificates, this should not happen!" << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
    }

    return true;
}
