#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <limits>

#include "global_funcs.h"
#include "task.h"
#include "timer.h"
#include "proofchecker.h"
#include "setformula.h"
#include "setformulacompound.h"
#include "setformulahorn.h"
#include "setformulabdd.h"
#include "setformulaexplicit.h"


void read_in_expression(std::ifstream &in, ProofChecker &proofchecker, Task *task) {
    FormulaIndex expression_index;
    in >> expression_index;
    std::string type;
    // read in expression type
    in >> type;
    SetFormula *expression;

    if(type == "b") {
        expression = new SetFormulaBDD(in, task);
    } else if(type == "h") {
        expression = new SetFormulaHorn(in, task);
    } else if(type == "t") {
        std::cerr << "not implemented yet" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    } else if(type == "e") {
        expression = new SetFormulaExplicit(in, task);
    } else if(type == "c") {
        expression = new SetFormulaConstant(in, task);
    } else if(type == "n") {
        FormulaIndex subformulaindex;
        in >> subformulaindex;
        expression = new SetFormulaNegation(subformulaindex);
    } else if(type == "i") {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        expression = new SetFormulaIntersection(left, right);
    } else if(type == "u") {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        expression = new SetFormulaUnion(left, right);
    } else if(type == "p") {
        FormulaIndex subformulaindex;
        in >> subformulaindex;
        expression = new SetFormulaProgression(subformulaindex);
    } else if(type == "r") {
        FormulaIndex subformulaindex;
        in >> subformulaindex;
        expression = new SetFormulaRegression(subformulaindex);
    } else {
        std::cerr << "unknown expression type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    proofchecker.add_formula(expression, expression_index);
}

void read_and_check_knowledge(std::ifstream &in, ProofChecker &proofchecker) {
    KnowledgeIndex knowledge_index;
    in >> knowledge_index;
    bool knowledge_is_correct = false;

    std::string word;
    // read in knowledge type
    in >> word;
    if(word == "s") {
        // subset knowledge requires two setids with the semantics "left is subset of right"
        FormulaIndex left, right;
        in >> left;
        in >> right;
        // read in with which basic statement or derivation rule this knowledge should be checked
        in >> word;
        if(word == "b1") {
            knowledge_is_correct = proofchecker.check_statement_B1(knowledge_index, left, right);
        } else if(word == "b2") {
            knowledge_is_correct = proofchecker.check_statement_B2(knowledge_index, left, right);
        } else if(word == "b3") {
            knowledge_is_correct = proofchecker.check_statement_B3(knowledge_index, left, right);
        } else if(word == "b4") {
            knowledge_is_correct = proofchecker.check_statement_B4(knowledge_index, left, right);
        } else if(word == "b5") {
            knowledge_is_correct = proofchecker.check_statement_B5(knowledge_index, left, right);
        } else if(word == "d10") {
            KnowledgeIndex old_knowledge_index;
            in >> old_knowledge_index;
            knowledge_is_correct = proofchecker.check_rule_D10(knowledge_index, left, right, old_knowledge_index);
        } else if(word == "d11") {
            KnowledgeIndex old_knowledge_index;
            in >> old_knowledge_index;
            knowledge_is_correct = proofchecker.check_rule_D11(knowledge_index, left, right, old_knowledge_index);
        } else {
            std::cerr << "unknown justification for subset knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
    } else if(word == "d") {
        // dead knowledge requires one setid telling which set is dead
        FormulaIndex dead_index;
        in >> dead_index;
        // read in with which derivation rule this knowledge should be checked
        in >> word;
        if(word == "d1") {
            knowledge_is_correct = proofchecker.check_rule_D1(knowledge_index, dead_index);
        } else if(word == "d2") {
            KnowledgeIndex ki1, ki2;
            in >> ki1;
            in >> ki2;
            knowledge_is_correct = proofchecker.check_rule_D2(knowledge_index, dead_index, ki1, ki2);
        } else if(word == "d3") {
            KnowledgeIndex ki1, ki2;
            in >> ki1;
            in >> ki2;
            knowledge_is_correct = proofchecker.check_rule_D3(knowledge_index, dead_index, ki1, ki2);
        } else if(word == "d6") {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            knowledge_is_correct = proofchecker.check_rule_D6(knowledge_index, dead_index, ki1, ki2, ki3);
        } else if(word == "d7") {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            knowledge_is_correct = proofchecker.check_rule_D7(knowledge_index, dead_index, ki1, ki2, ki3);
        } else if(word == "d8") {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            knowledge_is_correct = proofchecker.check_rule_D8(knowledge_index, dead_index, ki1, ki2, ki3);
        } else if(word == "d9") {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            knowledge_is_correct = proofchecker.check_rule_D9(knowledge_index, dead_index, ki1, ki2, ki3);
        }
    } else if(word == "u") {
        // read in with which derivation rule unsolvability should be proven
        in >> word;
        if(word == "d4") {
            KnowledgeIndex ki;
            in >> ki;
            knowledge_is_correct = proofchecker.check_rule_D4(knowledge_index, ki);
        } else if(word == "d5") {
            KnowledgeIndex ki;
            in >> ki;
            knowledge_is_correct = proofchecker.check_rule_D5(knowledge_index, ki);
        }
    } else {
        std::cerr << "unknown knowledge type " << word << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    if(!knowledge_is_correct) {
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
    int x = std::numeric_limits<int>::max();
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

    manager = Cudd(task->get_number_of_facts()*2);
    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();
    std::cout << "Amount of Actions: " << task->get_number_of_actions() << std::endl;

    if(certificate_file.compare(0,8,"$TMPDIR/") == 0) {
        std::string old_certificate_file = certificate_file;
        certificate_file = std::getenv("TMPDIR");
        certificate_file += "/";
        certificate_file += old_certificate_file.substr(8);
    }

    ProofChecker proofchecker;
    proofchecker.first_pass(certificate_file);

    std::ifstream certstream;
    certstream.open(certificate_file);
    if(!certstream.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }
    std::string input;
    while(certstream >> input) {
        // check if timeout is reached
        // TODO: we currently only check timeout here and in the Cudd manager. Is this sufficient?
        if(timer() > g_timeout) {
            exit_timeout("");
        }

        if(input.compare("e") == 0) {
            read_in_expression(certstream, proofchecker, task);
        } else if(input.compare("k") == 0) {
            read_and_check_knowledge(certstream, proofchecker);
        } else if(input.at(0) == '#') {
            // comment - ignore
            // TODO: is this safe even if the line conssits only of "#"? Or will it skip a line?
            std::getline(certstream, input);
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
