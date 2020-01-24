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
#include "stateset.h"
#include "setformulaconstant.h"
#include "setformulahorn.h"
#include "setformulabdd.h"
#include "setformulaexplicit.h"

enum class KnowledgeOrigin {
    BSUBSET,
    BSUBSETPROGRESSION,
    BSUBSETREGRESSION,
    BSUBSETMIXED,
    BACTION,
    EMPTYDEAD,
    UNIONDEAD,
    SUBSETDEAD,
    PROGRESSIONGOAL,
    PROGRESSIONINITIAL,
    REGRESSIONGOAL,
    REGRESSIONINITIAL,
    CONCLUSIONINITIAL,
    CONCLUSIONGOAL,
    UNIONRIGHT,
    UNIONLEFT,
    INTERSECTIONRIGHT,
    INTERSECTIONLEFT,
    DISTRIBUTIVITY,
    SUBSETUNION,
    SUBSETINTERSECTION,
    SUBSETTRANSITIVIY,
    ACTIONTRANSITIVITY,
    ACTIONUNION,
    PROGRESSIONTRANSITIVIY,
    PROGRESSIONUNION,
    PROGRESSIONTOREGRESSION,
    REGRESSIONTOPROGRESSION,
    NONE
};

void read_in_actionset(std::ifstream &in, ProofChecker &proofchecker, Task *task) {
    ActionSetIndex action_index;
    in >> action_index;
    std::string type;
    // read in action type
    in >> type;

    if(type == "b") { // basic enumeration of actions
        int amount;
        // the first number denotes the amount of actions being enumerated
        std::unordered_set<int> actions;
        in >> amount;
        int a;
        for(int i = 0; i < amount; ++i) {
            in >> a;
            actions.insert(a);
        }
        proofchecker.add_actionset(std::unique_ptr<ActionSet>(new ActionSetBasic(actions)),
                                   action_index);

    } else if(type == "u") { // union of action sets
        int left, right;
        in >> left;
        in >> right;
        proofchecker.add_actionset_union(left, right, action_index);

    } else if(type == "a") { // constant (denoting the set of all actions)
        proofchecker.add_actionset(std::unique_ptr<ActionSet>(new ActionSetConstantAll(task)),
                                   action_index);

    } else {
        std::cerr << "unknown actionset type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
}

void read_and_check_knowledge(std::ifstream &in, ProofChecker &proofchecker,
                              std::map<std::string, KnowledgeOrigin> origin_map) {
    KnowledgeIndex knowledge_index;
    in >> knowledge_index;
    bool knowledge_is_correct = false;

    std::string word;
    // read in knowledge type
    in >> word;

    if(word == "s") { // subset knowledge
        FormulaIndex left, right;
        in >> left;
        in >> right;
        // read in with which basic statement or derivation rule this knowledge should be checked
        in >> word;

        KnowledgeOrigin origin = KnowledgeOrigin::NONE;
        if (origin_map.find(word) != origin_map.end()) {
            origin = origin_map.at(word);
        }
        switch(origin) {
        case KnowledgeOrigin::BSUBSET:
            knowledge_is_correct = proofchecker.check_statement_B1(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::BSUBSETPROGRESSION:
            knowledge_is_correct = proofchecker.check_statement_B2(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::BSUBSETREGRESSION:
            knowledge_is_correct = proofchecker.check_statement_B3(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::BSUBSETMIXED:
            knowledge_is_correct = proofchecker.check_statement_B4(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::BACTION:
            knowledge_is_correct = proofchecker.check_statement_B5(knowledge_index, left, right);
            break;
        // TODO: new names according to thesis
        case KnowledgeOrigin::UNIONRIGHT:
            knowledge_is_correct = proofchecker.check_rule_ur(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::UNIONLEFT:
            knowledge_is_correct = proofchecker.check_rule_ul(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::INTERSECTIONRIGHT:
            knowledge_is_correct = proofchecker.check_rule_ir(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::INTERSECTIONLEFT:
            knowledge_is_correct = proofchecker.check_rule_il(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::DISTRIBUTIVITY:
            knowledge_is_correct = proofchecker.check_rule_di(knowledge_index, left, right);
            break;
        case KnowledgeOrigin::SUBSETUNION: {
            KnowledgeIndex premise1, premise2;
            in >> premise1;
            in >> premise2;
            knowledge_is_correct = proofchecker.check_rule_su(knowledge_index, left, right, premise1, premise2);
            break;
        }
        case KnowledgeOrigin::SUBSETINTERSECTION: {
            KnowledgeIndex premise1, premise2;
            in >> premise1;
            in >> premise2;
            knowledge_is_correct = proofchecker.check_rule_si(knowledge_index, left, right, premise1, premise2);
            break;
        }
        case KnowledgeOrigin::SUBSETTRANSITIVIY: {
            KnowledgeIndex premise1, premise2;
            in >> premise1;
            in >> premise2;
            knowledge_is_correct = proofchecker.check_rule_st(knowledge_index, left, right, premise1, premise2);
            break;
        }
        case KnowledgeOrigin::ACTIONTRANSITIVITY: {
            KnowledgeIndex premise1, premise2;
            in >> premise1;
            in >> premise2;
            knowledge_is_correct = proofchecker.check_rule_at(knowledge_index, left, right, premise1, premise2);
            break;
        }
        case KnowledgeOrigin::ACTIONUNION: {
            KnowledgeIndex premise1, premise2;
            in >> premise1;
            in >> premise2;
            knowledge_is_correct = proofchecker.check_rule_au(knowledge_index, left, right, premise1, premise2);
            break;
        }
        case KnowledgeOrigin::PROGRESSIONTRANSITIVIY: {
            KnowledgeIndex premise1, premise2;
            in >> premise1;
            in >> premise2;
            knowledge_is_correct = proofchecker.check_rule_pt(knowledge_index, left, right, premise1, premise2);
            break;
        }
        case KnowledgeOrigin::PROGRESSIONUNION: {
            KnowledgeIndex premise1, premise2;
            in >> premise1;
            in >> premise2;
            knowledge_is_correct = proofchecker.check_rule_pu(knowledge_index, left, right, premise1, premise2);
            break;
        }
        case KnowledgeOrigin::PROGRESSIONTOREGRESSION: {
            KnowledgeIndex premise;
            in >> premise;
            knowledge_is_correct = proofchecker.check_rule_pr(knowledge_index, left, right, premise);
            break;
        }
        case KnowledgeOrigin::REGRESSIONTOPROGRESSION: {
            KnowledgeIndex premise;
            in >> premise;
            knowledge_is_correct = proofchecker.check_rule_rp(knowledge_index, left, right, premise);
            break;
        }
        default:
            std::cerr << "unknown justification for subset knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }

    } else if(word == "d") { // dead knowledge
        FormulaIndex dead_index;
        in >> dead_index;
        // read in with which derivation rule this knowledge should be checked
        in >> word;

        KnowledgeOrigin origin = KnowledgeOrigin::NONE;
        if (origin_map.find(word) != origin_map.end()) {
            origin = origin_map.at(word);
        }
        // TODO: new names according to thesis
        switch(origin) {
        case KnowledgeOrigin::EMPTYDEAD:
            knowledge_is_correct = proofchecker.check_rule_ed(knowledge_index, dead_index);
            break;
        case KnowledgeOrigin::UNIONDEAD: {
            KnowledgeIndex ki1, ki2;
            in >> ki1;
            in >> ki2;
            knowledge_is_correct = proofchecker.check_rule_ud(knowledge_index, dead_index, ki1, ki2);
            break;
        }
        case KnowledgeOrigin::SUBSETDEAD: {
            KnowledgeIndex ki1, ki2;
            in >> ki1;
            in >> ki2;
            knowledge_is_correct = proofchecker.check_rule_sd(knowledge_index, dead_index, ki1, ki2);
            break;
        }
        case KnowledgeOrigin::PROGRESSIONGOAL: {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            knowledge_is_correct = proofchecker.check_rule_pg(knowledge_index, dead_index, ki1, ki2, ki3);
            break;
        }
        case KnowledgeOrigin::PROGRESSIONINITIAL: {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            knowledge_is_correct = proofchecker.check_rule_pi(knowledge_index, dead_index, ki1, ki2, ki3);
            break;
        }
        case KnowledgeOrigin::REGRESSIONGOAL: {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            knowledge_is_correct = proofchecker.check_rule_rg(knowledge_index, dead_index, ki1, ki2, ki3);
            break;
        }
        case KnowledgeOrigin::REGRESSIONINITIAL: {
            KnowledgeIndex ki1, ki2, ki3;
            in >> ki1;
            in >> ki2;
            in >> ki3;
            knowledge_is_correct = proofchecker.check_rule_ri(knowledge_index, dead_index, ki1, ki2, ki3);
            break;
        }
        default: {
            std::cerr << "unknown justification for dead set knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
        }

    } else if(word == "u") { // unsolvability knowledge
        // read in with which derivation rule unsolvability should be proven
        in >> word;

        KnowledgeOrigin origin = KnowledgeOrigin::NONE;
        if (origin_map.find(word) != origin_map.end()) {
            origin = origin_map.at(word);
        }
        switch(origin) {
        case KnowledgeOrigin::CONCLUSIONINITIAL: {
            KnowledgeIndex ki;
            in >> ki;
            knowledge_is_correct = proofchecker.check_rule_ci(knowledge_index, ki);
            break;
        }
        case KnowledgeOrigin::CONCLUSIONGOAL: {
            KnowledgeIndex ki;
            in >> ki;
            knowledge_is_correct = proofchecker.check_rule_cg(knowledge_index, ki);
            break;
        }
        default: {
            std::cerr << "unknown justification for dead set knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
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

    // expand environment variables in task file
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
    // expand environment variables in certificate file
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

    int timeout = std::numeric_limits<int>::max();
    bool discard_formulas = false;
    for(int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.compare("--discard_formulas") == 0) {
            discard_formulas = true;
            std::cout << "discarding formulas when not needed anymore" << std::endl;
        } else if (arg.substr(0,10).compare("--timeout=") == 0) {
            std::istringstream ss(arg.substr(10));
            if (!(ss >> timeout) || timeout < 0) {
                std::cout << "Usage: verify <task-file> <certificate-file> [--timeout=x] [--discard_formulas]" << std::endl;
                std::cout << "timeout is an optional parameter in seconds" << std::endl;
                exit(0);
            }
            std::cout << "using timeout of " << timeout << " seconds" << std::endl;
        } else {
            std::cout << "Usage: verify <task-file> <certificate-file> [--timeout=x] [--discard_formulas]" << std::endl;
            std::cout << "timeout is an optional parameter in seconds" << std::endl;
            exit(0);
        }
    }
    set_timeout(timeout);
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

    std::map<std::string, KnowledgeOrigin> knowledge_origins {
        { "b1", KnowledgeOrigin::BSUBSET },
        { "b2", KnowledgeOrigin::BSUBSETPROGRESSION },
        { "b3", KnowledgeOrigin::BSUBSETREGRESSION },
        { "b4", KnowledgeOrigin::BSUBSETMIXED },
        { "b5", KnowledgeOrigin::BACTION },
        { "ed", KnowledgeOrigin::EMPTYDEAD },
        { "ud", KnowledgeOrigin::UNIONDEAD },
        { "sd", KnowledgeOrigin::SUBSETDEAD },
        { "pg", KnowledgeOrigin::PROGRESSIONGOAL },
        { "pi", KnowledgeOrigin::PROGRESSIONINITIAL },
        { "rg", KnowledgeOrigin::REGRESSIONGOAL },
        { "ri", KnowledgeOrigin::REGRESSIONINITIAL },
        { "ci", KnowledgeOrigin::CONCLUSIONINITIAL },
        { "cg", KnowledgeOrigin::CONCLUSIONGOAL },
        { "ur", KnowledgeOrigin::UNIONRIGHT },
        { "ul", KnowledgeOrigin::UNIONLEFT },
        { "ir", KnowledgeOrigin::INTERSECTIONRIGHT },
        { "il", KnowledgeOrigin::INTERSECTIONLEFT },
        { "di", KnowledgeOrigin::DISTRIBUTIVITY },
        { "su", KnowledgeOrigin::SUBSETUNION },
        { "si", KnowledgeOrigin::SUBSETINTERSECTION },
        { "st", KnowledgeOrigin::SUBSETTRANSITIVIY },
        { "at", KnowledgeOrigin::ACTIONTRANSITIVITY },
        { "au", KnowledgeOrigin::ACTIONUNION },
        { "pt", KnowledgeOrigin::PROGRESSIONTRANSITIVIY },
        { "pu", KnowledgeOrigin::PROGRESSIONUNION },
        { "pr", KnowledgeOrigin::PROGRESSIONTOREGRESSION },
        { "rp", KnowledgeOrigin::REGRESSIONTOPROGRESSION }
    };

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
            proofchecker.add_expression(certstream, task);
        } else if(input.compare("k") == 0) {
            read_and_check_knowledge(certstream, proofchecker, knowledge_origins);
        } else if(input.compare("a") == 0) {
            read_in_actionset(certstream, proofchecker, task);
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
