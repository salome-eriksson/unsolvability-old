#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <stdlib.h>

#include "task.h"
#include "certificate.h"
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
        std::cout << "invalid certificate file" << std::endl;
        exit(0);
    }
    std::string line;
    std::getline(stream, line);

    if(line.compare("simple_certificate") == 0) {
        std::cout << "reading in simple certificate" << std::endl;
        return new SimpleCertificate(task, stream);
    } else if(line.compare("strong_conjunctive_certificate") == 0) {
        //TODO
        std::cout << "not yet implemented (strong conjunctive certificate)" << std::endl;
        exit(0);
    } else if(line.compare("search_certificate") == 0) {
        std::cout << "reading in search certificate" << std::endl;
        return new SearchCertificate(task, stream);
    } else {
        std::cout << "unknown certificate type: " << line << std::endl;
        exit(0);
    }
    return NULL;
}



int main(int argc, char** argv) {
    if(argc < 3) {
        std::cout << "Usage: verify <pddl-file> <certificate-file>" << std::endl;
        exit(0);
    }
    initialize_timer();
    std::string pddl_file = argv[1];
    std::string certificate_file = argv[2];
    std::cout << "reading in task" << std::endl;
    Task* task = new Task(pddl_file);
    Certificate* certificate = build_certificate(certificate_file, task);
    bool valid = certificate->is_certificate_for(task->get_initial_state());
    if(valid) {
        std::cout << "The certificate is valid" << std::endl;
    }
    exit(0);
}
