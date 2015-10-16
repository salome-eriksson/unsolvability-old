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

/* TODO: clean up includes! */

Certificate* parse_certificate(std::string file, Task* task) {
    std::ifstream stream;
    stream.open(file.c_str());
    if(!stream.is_open()) {
        std::cout << "invalid certificate file" << std::endl;
        exit(0);
    }
    std::string line;
    std::getline(stream, line);
    std::vector<std::string> elems;
    split(line, elems, ':');
    if(elems.size() == 0 || elems[0].compare("Number of certificates") != 0) {
        print_parsing_error_and_exit(line, "Number of certificates:<#certificates>");
    }
    int number_of_certificates = atoi(elems[1].c_str());
    //TODO: cannot add NULL as default value?
    std::vector<Certificate*> certificates(number_of_certificates);
    for(int i = 0; i < number_of_certificates; ++i) {
        std::getline(stream, line);
        if(line.compare("start_certificate") != 0) {
            print_parsing_error_and_exit(line, "start_certificate");
        }
        std::string certificate_type;
        std::vector<std::string> certificate_info;
        std::getline(stream, certificate_type);
        std::getline(stream, line);
        while(line.compare("end_certificate") != 0) {
            certificate_info.push_back(line);
            if(!std::getline(stream, line)) {
                std::cout << "file ended without reading \"end_certificate\"" << std::endl;
                exit(0);
            }
        }

        if(certificate_type.compare("Simple certificate")==0) {
            certificates[i] = new SimpleCertificate(task, certificate_info);
        } else if(certificate_type.compare("Strong conjunctive certificate")==0) {
            /*TODO*/
            std::cout << "not yet implemented (strong conjunctive certificate)" << std::endl;
            exit(0);
        } else if(certificate_type.compare("Search certificate")==0) {
            std::cout << "not yet implemented (search certificate)" << std::endl;
            exit(0);
            /*TODO*/
        } else {
            std::cout << "unknown certificate type: " << line << std::endl;
            exit(0);
        }

    }
    //assert that file is read entirely now and that at least 1 certificate has been read
    assert(!std::getline(stream, line));
    assert(certificates[0] != NULL);
    return certificates[0];
}



int main(int argc, char** argv) {
  if(argc < 3) {
      std::cout << "Usage: verify <pddl-file> <certificate-file>" << std::endl;
      exit(0);
  }
  std::string pddl_file = argv[1];
  std::string certificate_file = argv[2];
  Task* task = new Task(pddl_file);
  Certificate* certificate = parse_certificate(certificate_file, task);
  bool valid = certificate->is_unsolvability_certificate();
  if(valid) {
      std::cout << "The certificate is valid" << std::endl;
  } else {
      std::cout << "The certificate is NOT valid" << std::endl;
  }
  exit(0);
}
