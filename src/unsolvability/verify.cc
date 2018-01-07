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
#include "proofchecker.h"
#include "setformula.h"
#include "setformulacompound.h"
#include "setformulahorn.h"


void read_in_expression(std::ifstream &in, ProofChecker &proofchecker, Task *task) {
    FormulaIndex expression_index;
    in >> expression_index;
    std::string type;
    in >> type;
    SetFormula *expression;

    if(type.compare("b") == 0) {
        std::cerr << "not implemented yet" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    } else if(type.compare("h") == 0) {
        expression = new SetFormulaHorn(in, task);
    } else if(type.compare("t") == 0) {
        std::cerr << "not implemented yet" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    } else if(type.compare("e") == 0) {
        std::cerr << "not implemented yet" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    } else if(type.compare("c") == 0) {
        expression = new SetFormulaConstant(in, task);
    } else if(type.compare("n") == 0) {
        FormulaIndex subformulaindex;
        in >> subformulaindex;
        expression = new SetFormulaNegation(subformulaindex);
    } else if(type.compare("i") == 0) {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        expression = new SetFormulaIntersection(left, right);
    } else if(type.compare("u") == 0) {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        expression = new SetFormulaUnion(left, right);
    } else if(type.compare("p") == 0) {
        FormulaIndex subformulaindex;
        in >> subformulaindex;
        expression = new SetFormulaProgression(subformulaindex);
    } else if(type.compare("r") == 0) {
        FormulaIndex subformulaindex;
        in >> subformulaindex;
        expression = new SetFormulaRegression(subformulaindex);
    } else {
        std::cerr << "unknown expression type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    proofchecker.add_formula(expression, expression_index);
}

void read_in_knowledge(std::ifstream &in, ProofChecker &proofchecker) {
    KnowledgeIndex knowledge_index;
    in >> knowledge_index;
    bool successful;

    std::string type;
    in >> type;
    if(type.compare("s") == 0) {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        in >> type;
        if(type.compare("b1") == 0) {
            successful = proofchecker.check_statement_B1(knowledge_index, left, right);
        } else if(type.compare("b2") == 0) {
            successful = proofchecker.check_statement_B2(knowledge_index, left, right);
        } else if(type.compare("b3") == 0) {
            successful = proofchecker.check_statement_B3(knowledge_index, left, right);
        } else if(type.compare("b4") == 0) {
            successful = proofchecker.check_statement_B4(knowledge_index, left, right);
        } else if(type.compare("b5") == 0) {
            successful = proofchecker.check_statement_B5(knowledge_index, left, right);
        } else if(type.compare("d10") == 0) {
            KnowledgeIndex old_knowledge_index;
            in >> old_knowledge_index;
            successful = proofchecker.check_rule_D10(knowledge_index, left, right, old_knowledge_index);
        } else if(type.compare("d11") == 0) {
            KnowledgeIndex old_knowledge_index;
            in >> old_knowledge_index;
            successful = proofchecker.check_rule_D11(knowledge_index, left, right, old_knowledge_index);
        } else {
            std::cerr << "unknown justification for subset knowledge " << type << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
    } else if(type.compare("d") == 0) {
        FormulaIndex dead_index;
        in >> dead_index;
        in >> type;
        if(type.compare("d1") == 0) {
            successful = proofchecker.check_rule_D1(knowledge_index, dead_index);
        } else if(type.compare("d2") == 0) {
            KnowledgeIndex ki1, ki2;
            in >> ki1;
            in >> ki2;
            successful = proofchecker.check_rule_D2(knowledge_index, dead_index, ki1, ki2);
        } else if(type.compare("d3") == 0) {
            KnowledgeIndex ki1, ki2;
            in >> ki1;
            in >> ki2;
            successful = proofchecker.check_rule_D3(knowledge_index, dead_index, ki1, ki2);
        } else if(type.compare("d6") == 0) {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            successful = proofchecker.check_rule_D6(knowledge_index, dead_index, ki1, ki2, ki3);
        } else if(type.compare("d7") == 0) {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            successful = proofchecker.check_rule_D7(knowledge_index, dead_index, ki1, ki2, ki3);
        } else if(type.compare("d8") == 0) {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            successful = proofchecker.check_rule_D8(knowledge_index, dead_index, ki1, ki2, ki3);
        } else if(type.compare("d9") == 0) {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            successful = proofchecker.check_rule_D9(knowledge_index, dead_index, ki1, ki2, ki3);
        }
    } else {
        std::cerr << "unknown knowledge type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    if(!successful) {
        std::cerr << "check for knowledge #" << knowledge_index << " NOT successful!" << std::endl;
    }
}

int main(int argc, char** argv) {
    if(argc < 3 || argc > 4) {
        std::cout << "Usage: verify <task-file> <certificate-file> [timeout]" << std::endl;
        std::cout << "timeout is an optional parameter in seconds" << std::endl;
        exit(0);
    }
    register_event_handlers();
    initialize_timer();
    setup_hex();
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

    if(certificate_file.compare(0,8,"$TMPDIR/") == 0) {
        std::string old_certificate_file = certificate_file;
        certificate_file = std::getenv("TMPDIR");
        certificate_file += "/";
        certificate_file += old_certificate_file.substr(8);
    }

    std::ifstream certstream;
    certstream.open(certificate_file);
    if(!certstream.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }
    ProofChecker proofchecker;
    std::string input;
    while(certstream >> input) {
        if(input.compare("e") == 0) {
            read_in_expression(certstream, proofchecker, task);
        } else if(input.compare("k") == 0) {
            read_in_knowledge(certstream, proofchecker);
        } else {
            std::cerr << "unknown start of line: " << input << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
    }

    std::cout << "Verify total time: " << timer() << std::endl;
    std::cout << "Verify memory: " << get_peak_memory_in_kb() << "KB" << std::endl;
    if(proofchecker.is_unsolvability_proven()) {
        std::cout << "unsolvability proven" << std::endl;
        exit_with(ExitCode::CERTIFICATE_VALID);
    } else {
        std::cout << "unsolvability NOT proven" << std::endl;
        exit_with(ExitCode::CERTIFICATE_NOT_VALID);
    }
}
