#include "statementchecker.h"
#include "global_funcs.h"

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

void StatementChecker::check_statements_and_insert_into_kb() {
    assert(statementfile.compare("") != 0);
    std::ifstream in;
    in.open(statementfile);

    std::string line;
    while(std::getline(in, line)) {
        bool statement_correct = false;
        int pos_colon = line.find(":");
        assert(string_to_statement.find(line.substr(0, pos_colon)) != string_to_statement.end());
        Statement statement = string_to_statement.find(line.substr(0, pos_colon))->second;
        std::vector<std::string> params = determine_parameters(line.substr(pos_colon+1), ';');
        switch(statement) {
        case Statement::SUBSET: {
            assert(params.size() == 2);
            statement_correct = check_subset(params[0], params[1]);
            if(statement_correct) {
                kb->insert_subset(params[0],params[1]);
            }
            break;
        }
        case Statement::EXPLICIT_SUBSET: {
            assert(params.size() == 2);
            statement_correct = check_set_subset_to_stateset(params[0],kb->get_state_set(params[1]));
            if(statement_correct) {
                kb->insert_subset(params[0],kb->get_state_set(params[1]).getName());
            }
            break;
        }
        case Statement::PROGRESSION: {
            assert(params.size() == 2);
            statement_correct = check_progression(params[0],params[1]);
            if(statement_correct) {
                kb->insert_progression(params[0],params[1]);
            }
            break;
        }
        case Statement::REGRESSION: {
            assert(params.size() == 2);
            statement_correct = check_regression(params[0],params[1]);
            if(statement_correct) {
                kb->insert_regression(params[0],params[1]);
            }
            break;
        }
        case Statement::CONTAINED: {
            assert(params.size() == 2);
            Cube state_cube = parseCube(params[0], task->get_number_of_facts());
            statement_correct = check_is_contained(state_cube, params[1]);
            if(statement_correct) {
                kb->insert_contained_in(state_cube,params[1]);
            }
            break;
        }
        case Statement::INITIAL_CONTAINED: {
            assert(params.size() == 1);
            statement_correct = check_initial_contained(params[0]);
            if(statement_correct) {
                Cube initcube = task->get_initial_state();
                kb->insert_contains_initial(params[0]);
                kb->insert_contained_in(initcube,params[0]);
            }
            break;
        }
        default: {
            std::cout << "unkown statement: " << statement;
            break;
        }
        }
        if(!statement_correct) {
            std::cout << "statement NOT correct: " << line << std::endl;
        }
    }
}