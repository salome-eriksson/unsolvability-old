#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <stdlib.h>
#include <iomanip>

#include "certificate.h"
#include "conjunctive_certificate.h"
#include "disjunctive_certificate.h"
#include "global_funcs.h"
#include "simple_certificate.h"
#include "task.h"
#include "timer.h"

Certificate* build_certificate(std::string certificate_file, Task* task) {
    std::ifstream stream;
    stream.open(certificate_file);
    if(!stream.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }

    std::string line;
    std::getline(stream, line);
    std::vector<std::string> linevec;
    split(line, linevec, ':');
    assert(linevec.size() == 3);
    assert(linevec[0].compare("certificate-type") == 0);
    std::string type = linevec[1];
    int r = stoi(linevec[2]);

    Certificate *certificate = NULL;

    if(type.compare("simple") == 0) {
        std::cout << "reading in simple certificate" << std::endl;
        certificate = new SimpleCertificate(task, stream);
    } else if(type.compare("disjunctive") == 0) {
        std::cout << "reading in disjunctive certificate (bound:" << r << ")" << std::endl;
        certificate = new DisjunctiveCertificate(task, stream, r);
    } else if(type.compare("conjunctive") == 0) {
        std::cout << "reading in conjunctive certificate";
        certificate = new ConjunctiveCertificate(task, stream, r);
    } else {
        exit_with(ExitCode::PARSING_ERROR);
    }
    stream.close();
    return certificate;
}



int main(int argc, char** argv) {
    if(argc < 3 || argc > 4) {
        std::cout << "Usage: verify <task-file> <certificate-file> [timeout]" << std::endl;
        std::cout << "timeout is an optional parameter in seconds" << std::endl;
        exit(0);
    }
    register_event_handlers();
    initialize_timer();
    std::string task_file = argv[1];
    std::string certificate_file = argv[2];

    // expand environment variables
    size_t found = task_file.find('$');
    while(found != std::string::npos) {
        size_t end = task_file.find('/');
        std::string envvar;
        if(end == std::string::npos) {
            envvar = task_file.substr(found+1);
        } else {
            envvar = task_file.substr(found+1,end-found-1);
        }
        // to upper case
        for(size_t i = 0; i < envvar.size(); i++) {
            envvar.at(i) = toupper(envvar.at(i));
        }
        std::string expanded = std::getenv(envvar.c_str());
        task_file.replace(found,envvar.length()+1,expanded);
        found = task_file.find('$');
    }
    found = certificate_file.find('$');
    while(found != std::string::npos) {
        size_t end = certificate_file.find('/');
        std::string envvar;
        if(end == std::string::npos) {
            envvar = certificate_file.substr(found+1);
        } else {
            envvar = certificate_file.substr(found+1,end-found-1);
        }
        // to upper case
        for(size_t i = 0; i < envvar.size(); i++) {
            envvar.at(i) = toupper(envvar.at(i));
        }
        std::string expanded = std::getenv(envvar.c_str());
        certificate_file.replace(found,envvar.length()+1,expanded);
        found = certificate_file.find('$');
    }

    int x = 0;
    if(argc == 4) {
        std::istringstream ss(argv[3]);
        if (!(ss >> x) || x < 0) {
            std::cout << "Usage: verify <pddl-file> <certificate-file> [timeout]" << std::endl;
            std::cout << "timeout is an optional parameter in seconds" << std::endl;
            exit(0);
        }
        std::cout << "using timeout of " << x << " seconds" << std::endl;
    }
    set_timeout(x);
    print_info("Starting parsing");
    double parsing_start = timer();
    Task* task = new Task(task_file);
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
