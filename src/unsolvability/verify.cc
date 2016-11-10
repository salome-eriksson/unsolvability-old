#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <stdlib.h>
#include <iomanip>

#include "task.h"
#include "certificate.h"
#include "disjunctive_certificate.h"
#include "simple_certificate.h"
#include "strong_conjunctive_certificate.h"
#include "search_certificate.h"
#include "global_funcs.h"
#include "timer.h"

/* TODO: clean up includes! */

Certificate* build_certificate(std::string file, Task* task) {
    std::ifstream stream;
    stream.open(file.c_str());
    if(!stream.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }
    std::string line;
    std::getline(stream, line);

    if(line.compare("simple_certificate") == 0) {
        std::cout << "reading in simple certificate" << std::endl;
        return new SimpleCertificate(task, stream);
    } else if(line.compare("strong_conjunctive_certificate") == 0) {
        std::cout << "reading in strong conjunctive certificate" << std::endl;
        return new StrongConjunctiveCertificate(task, stream);
    } else if(line.compare("search_certificate") == 0) {
        std::cout << "reading in search certificate" << std::endl;
        return new SearchCertificate(task, stream);
    } else if(line.compare("disjunctive_certificate") == 0) {
        std::cout << "reading in disjunctive certificate" << std::endl;
        return new DisjunctiveCertificate(task, stream);
    } else {
        exit_with(ExitCode::PARSING_ERROR);
    }
    return NULL;
}



int main(int argc, char** argv) {
    if(argc < 3 || argc > 4) {
        std::cout << "Usage: verify <pddl-file> <certificate-file> [timeout]" << std::endl;
        std::cout << "timeout is an optional parameter in seconds" << std::endl;
        exit(0);
    }
    initialize_timer();
    extern DD_OOMFP MMoutOfMemory;
    MMoutOfMemory = exit_oom;
    std::string pddl_file = argv[1];
    std::string certificate_file = argv[2];
    int x = 0;
    if(argc == 4) {
        std::istringstream ss(argv[3]);
        if (!(ss >> x) || x < 0) {
            std::cout << "Usage: verify <pddl-file> <certificate-file> [timeout]" << std::endl;
            std::cout << "timeout is an optional parameter in seconds" << std::endl;
            exit(0);
        }
        std::cout << "using timeout of " << g_timeout << " seconds" << std::endl;
    }
    set_timeout(x);
    print_info("Starting parsing");
    double parsing_start = timer();
    Task* task = new Task(pddl_file);
    Certificate* certificate = build_certificate(certificate_file, task);
    double parsing_end = timer();
    print_info("Finished parsing");
    std::cout << "Amount of Actions: " << task->get_number_of_actions() << std::endl;
    print_info("Verifying certificate");
    double verify_start = timer();
    bool valid = certificate->is_certificate_for(task->get_initial_state());
    double verify_end =  timer();
    double parsing_time = parsing_end-parsing_start;
    double verification_time = verify_end - verify_start;
    std::cout << "Verify total time: " << timer() << std::endl;
    std::cout << "Verify parsing time: ";
    std::cout << std::fixed << std::setprecision(2) << parsing_time << std::endl;
    std::cout << "Verify verification time: ";
    std::cout << std::fixed << std::setprecision(2) << verification_time << std::endl;
    std::cout << "Verify memory: " << get_peak_memory_in_kb() << "KB" << std::endl;
    if(valid) {
        exit_with(ExitCode::CERTIFICATE_VALID);
    } else {
        exit_with(ExitCode::CERTIFICATE_NOT_VALID);
    }
}
