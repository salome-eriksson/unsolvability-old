#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <stdlib.h>
#include <iomanip>


#include "global_funcs.h"
#include "task.h"
#include "timer.h"
#include "knowledgebase.h"
#include "rulechecker.h"
#include "statementchecker.h"
#include "statementcheckerbdd.h"
#include "statementcheckerhorn.h"

/*Certificate* build_certificate(std::string certificate_file, Task* task) {
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
}*/



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

    Task* task = new Task(task_file);
    std::cout << "Amount of Actions: " << task->get_number_of_actions() << std::endl;

    std::ifstream infofile;
    infofile.open(certificate_file);
    if(!infofile.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }
    std::string line;

    // first line contains location of the stateseet file needed for the knowledgebase
    std::getline(infofile,line);
    int pos = line.find(":");
    assert(pos != std::string::npos);
    assert(line.substr(0,pos).compare("statesets") == 0);
    KnowledgeBase *kb = new KnowledgeBase(task,line.substr(pos+1));

    // second line contains location of the rules file
    std::getline(infofile,line);
    pos = line.find(":");
    assert(pos != std::string::npos);
    assert(line.substr(0,pos).compare("rules") == 0);
    std::string rules_file = line.substr(pos+1);

    // loop over all statement blocks
    while(std::getline(infofile,line)) {
        StatementChecker *stmtchecker;
        if(line.compare("Statements:BDD") == 0) {
            stmtchecker = new StatementCheckerBDD(kb,task,infofile);
        } else if(line.compare("Statements:Horn") == 0) {
            stmtchecker = new StatementCheckerHorn(kb,task,infofile);
        } else {
            std::cout << "unknown Statement Type: " << line << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
        stmtchecker->check_statements_and_insert_into_kb();
        delete stmtchecker;
    }

    // check rules
    RuleChecker rulechecker(kb,task);
    rulechecker.check_rules_from_file(rules_file);

    std::cout << "Verify total time: " << timer() << std::endl;
    std::cout << "Verify memory: " << get_peak_memory_in_kb() << "KB" << std::endl;
    if(kb->is_unsolvability_proven()) {
        std::cout << "unsolvability proven" << std::endl;
        exit_with(ExitCode::CERTIFICATE_VALID);
    } else {
        std::cout << "unsolvability NOT proven" << std::endl;
        exit_with(ExitCode::CERTIFICATE_NOT_VALID);
    }

    //std::cout << "Verify verification time: ";
    //std::cout << std::fixed << std::setprecision(2) << verification_time << std::endl;
    std::cout << "Verify memory: " << get_peak_memory_in_kb() << "KB" << std::endl;

    //Certificate* certificate = build_certificate(certificate_file, task);
    //double parsing_end = timer();
    //print_info("Finished parsing");
    //print_info("Verifying certificate");
    //double verify_start = timer();
    //bool valid = certificate->is_certificate_for(task->get_initial_state());
    //double verify_end =  timer();
    //double parsing_time = parsing_end-parsing_start;
    //double verification_time = verify_end - verify_start;
    //std::cout << "Verify total time: " << timer() << std::endl;
    //std::cout << "Verify parsing time: ";
    //std::cout << std::fixed << std::setprecision(2) << parsing_time << std::endl;


}
