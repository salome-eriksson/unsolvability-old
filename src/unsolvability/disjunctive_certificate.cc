#include "disjunctive_certificate.h"

#include "dddmp.h"
#include "cudd.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cassert>
#include <stdlib.h>

DisjunctiveCertificate::DisjunctiveCertificate(Task *task, std::ifstream &stream)
    :  Certificate(task), cert_stream(stream) {

    //oftenly used variables
    std::string line;
    std::vector<std::string> line_arr;

    // parse bdd file
    std::getline(stream, line);
    split(line, line_arr, ':');
    if(line_arr[0].compare("State BDDs") != 0) {
        print_parsing_error_and_exit(line, "State BDDs:<state bdd file>");
    }
    assert(line_arr.size() == 2);
    std::string state_bdds_file = line_arr[1];
    std::cout << "State BDDs file: " << state_bdds_file << std::endl;
    store_bdds(state_bdds_file);

    // parse h cert bdd file
    std::getline(stream, line);
    line_arr.clear();
    split(line, line_arr, ':');
    if(line_arr[0].compare("Heuristic Certificates BDDs") != 0) {
        print_parsing_error_and_exit(line, "Heuristic Certificates BDDs:<h_cert bdd file>");
    }
    assert(line_arr.size() == 2);
    std::string hcert_bdds_file = line_arr[1];
    std::cout << "Heuristic Certificates BDDs file: " << hcert_bdds_file << std::endl;
    store_bdds(hcert_bdds_file);

    std::getline(stream, line);
    if(line.compare("begin hints") != 0) {
        print_parsing_error_and_exit(line, "begin hints");
    }
}

void DisjunctiveCertificate::store_bdds(std::string file) {
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

// stubs

bool DisjunctiveCertificate::contains_state(const Cube &) {
    return false;
}

bool DisjunctiveCertificate::contains_goal() {
    return false;
}

bool DisjunctiveCertificate::is_inductive() {
    return false;
}
