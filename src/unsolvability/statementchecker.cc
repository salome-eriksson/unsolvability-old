#include "statementchecker.h"

#include <cassert>
#include <sstream>
#include <iostream>


StatementChecker::StatementChecker(KnowledgeBase *kb, Task *task)
    : kb(kb), task(task) {

    string_to_statement.insert(std::make_pair("sub", Statement::SUBSET));
    string_to_statement.insert(std::make_pair("exsub", Statement::EXPLICIT_SUBSET));
    string_to_statement.insert(std::make_pair("prog", Statement::PROGRESSION));
    string_to_statement.insert(std::make_pair("reg", Statement::REGRESSION));
    string_to_statement.insert(std::make_pair("in", Statement::CONTAINED));
    string_to_statement.insert(std::make_pair("init", Statement::INITIAL_CONTAINED));

}

// TODO: this is a duplicate from RuleChecker::determine_parameters
std::vector<std::string> StatementChecker::determine_parameters(const std::string &parameter_line, char delim) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = parameter_line.find(";", delim);
        if (pos == std::string::npos) {
            pos = parameter_line.length();
        }
        std::string token = parameter_line.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + 1;
    }
    while (pos < parameter_line.length() && prev < parameter_line.length());
    return tokens;
}

// TOOD: this is a duplicate from RuleChecker::parseCube)
Cube StatementChecker::parseCube(const std::string &param) {
    Cube cube;
    cube.reserve(task->get_number_of_facts());
    std::istringstream iss(param);
    int n;
    while (iss >> n){
        cube.push_back(n);
    }
    assert(cube.size() == task->get_number_of_facts());
    return cube;
}

void StatementChecker::read_in_statements(std::ifstream &in) {
    std::string line;
    std::getline(in, line);
    while(line.compare("statements end") != 0) {
        bool statement_correct = false;
        int pos_colon = line.find(":");
        assert(string_to_statement.find(line.substr(0, pos_colon)) != string_to_statement.end());
        Statement statement = string_to_statement.find(line.substr(0, pos_colon))->second;
        std::vector<std::string> params = determine_parameters(line.substr(pos_colon+1), ';');
        switch(statement) {
        case Statement::SUBSET: {
            assert(params.size() == 2);
            statement_correct = check_subset(params[0], params[1]);
            break;
        }
        case Statement::EXPLICIT_SUBSET: {
            assert(params.size() == 2);
            statement_correct = check_set_subset_to_stateset(params[0],kb->get_state_set(params[1]));
            break;
        }
        case Statement::PROGRESSION: {
            assert(params.size() == 2);
            statement_correct = check_progression(params[0],params[1]);
            break;
        }
        case Statement::REGRESSION: {
            assert(params.size() == 2);
            statement_correct = check_regression(params[0],params[1]);
            break;
        }
        case Statement::CONTAINED: {
            assert(params.size() == 2);
            Cube state_cube = parseCube(params[0]);
            statement_correct = check_is_contained(state_cube, params[1]);
            break;
        }
        case Statement::INITIAL_CONTAINED: {
            assert(params.size() == 1);
            statement_correct = check_initial_contained(params[0]);
            break;
        }
        default: {
            std::cerr << "unkown statement: " << statement;
            break;
        }
        }
        if(!statement_correct) {
            std::cerr << "statement not correct: " << line;
        }
    }
}
