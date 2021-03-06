#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <limits>
#include <memory>
#include <regex>

#include "global_funcs.h"
#include "task.h"
#include "timer.h"
#include "proofchecker.h"
#include "setformula.h"
#include "setformulacompound.h"
#include "setformulahorn.h"
#include "setformuladualhorn.h"
#include "setformulabdd.h"
#include "setformulaexplicit.h"


void read_in_expression(std::ifstream &in, ProofChecker &proofchecker, Task *task) {
    FormulaIndex expression_index;
    in >> expression_index;
    std::string type;
    // read in expression type
    in >> type;
    std::unique_ptr<SetFormula> expression;

    if(type == "b") {
        expression = std::unique_ptr<SetFormula>(new SetFormulaBDD(in, task));
    } else if(type == "h") {
        expression = std::unique_ptr<SetFormula>(new SetFormulaHorn(in, task));
    } else if(type == "d") {
        expression = std::unique_ptr<SetFormula>(new SetFormulaDualHorn(in, task));
    } else if(type == "t") {
        std::cerr << "not implemented yet" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    } else if(type == "e") {
        expression = std::unique_ptr<SetFormula>(new SetFormulaExplicit(in, task));
    } else if(type == "c") {
        expression = std::unique_ptr<SetFormula>(new SetFormulaConstant(in, task));
    } else if(type == "n") {
        FormulaIndex subformulaindex;
        in >> subformulaindex;
        expression = std::unique_ptr<SetFormula>(new SetFormulaNegation(subformulaindex));
    } else if(type == "i") {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        expression = std::unique_ptr<SetFormula>(new SetFormulaIntersection(left, right));
    } else if(type == "u") {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        expression = std::unique_ptr<SetFormula>(new SetFormulaUnion(left, right));
    } else if(type == "p") {
        FormulaIndex subformulaindex, actionsetindex;
        in >> subformulaindex;
        in >> actionsetindex;
        expression = std::unique_ptr<SetFormula>(new SetFormulaProgression(subformulaindex, actionsetindex));
    } else if(type == "r") {
        FormulaIndex subformulaindex, actionsetindex;
        in >> subformulaindex;
        in >> actionsetindex;
        expression = std::unique_ptr<SetFormula>(new SetFormulaRegression(subformulaindex, actionsetindex));
    } else {
        std::cerr << "unknown expression type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    proofchecker.add_formula(std::move(expression), expression_index);
}

void read_in_actionset(std::ifstream &in, ProofChecker &proofchecker, Task *task) {
    ActionSetIndex action_index;
    in >> action_index;
    std::string type;
    // read in action type
    in >> type;
    if(type == "b") {
        int amount;
        std::unordered_set<int> actions;
        in >> amount;
        int a;
        for(int i = 0; i < amount; ++i) {
            in >> a;
            actions.insert(a);
        }
        proofchecker.add_actionset(std::unique_ptr<ActionSet>(new ActionSetBasic(actions)),
                                   action_index);
    } else if(type == "u") {
        int left, right;
        in >> left;
        in >> right;
        proofchecker.add_actionset_union(left, right, action_index);
    } else if(type == "a") {
        proofchecker.add_actionset(std::unique_ptr<ActionSet>(new ActionSetConstantAll(task)),
                                   action_index);
    } else {
        std::cerr << "unknown actionset type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
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
    if(argc < 3 || argc > 5) {
        std::cout << "Usage: verify <task-file> <certificate-file> [--timeout=x] [--discard_formulas]" << std::endl;
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

    int x = std::numeric_limits<int>::max();
    bool discard_formulas = false;
    for(int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.compare("--discard_formulas") == 0) {
            discard_formulas = true;
            std::cout << "discarding formulas when not needed anymore" << std::endl;
        } else if (arg.substr(0,10).compare("--timeout=") == 0) {
            std::istringstream ss(arg.substr(10));
            if (!(ss >> x) || x < 0) {
                std::cout << "Usage: verify <task-file> <certificate-file> [--timeout=x] [--discard_formulas]" << std::endl;
                std::cout << "timeout is an optional parameter in seconds" << std::endl;
                exit(0);
            }
            std::cout << "using timeout of " << x << " seconds" << std::endl;
        } else {
            std::cout << "Usage: verify <task-file> <certificate-file> [--timeout=x] [--discard_formulas]" << std::endl;
            std::cout << "timeout is an optional parameter in seconds" << std::endl;
            exit(0);
        }
    }
    set_timeout(x);
    set_discard_formulas(discard_formulas);
    Task* task = new Task(task_file);

    manager = Cudd(task->get_number_of_facts()*2);
    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();
    std::cout << "Amount of Actions: " << task->get_number_of_actions() << std::endl;

    ProofChecker proofchecker;
    if (discard_formulas) {
        proofchecker.first_pass(certificate_file);
    }

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
        } else if(input.compare("a") == 0) {
            read_in_actionset(certstream, proofchecker, task);
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
