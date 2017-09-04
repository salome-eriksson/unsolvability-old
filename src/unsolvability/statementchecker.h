#ifndef STATEMENTCHECKER_H
#define STATEMENTCHECKER_H

#include "knowledgebase.h"
#include "stateset.h"
#include "task.h"

enum Statement {
    SUBSET = "sub",
    EXPLICIT_SUBSET = "exsub",
    PROGRESSION = "prog",
    REGRESSION = "reg",
    CONTAINED = "in",
    INITIAL_CONTAINED = "init"
};

class StatementChecker
{
protected:
    KnowledgeBase *kb;
    Task *task;
    // TODO: this is a duplicate from RuleChecker::determine_parameters
    std::vector<std::string> determine_parameters(const std::string &parameter_line) {
        std::vector<std::string> tokens;
        size_t prev = 0, pos = 0;
        do
        {
            pos = parameter_line.find(";", prev);
            if (pos == std::string::npos) {
                pos = parameter_line.length();
            }
            std::string token = parameter_line.substr(prev, pos-prev);
            if (!token.empty()) tokens.push_back(token);
            prev = pos + delim.length();
        }
        while (pos < parameter_line.length() && prev < parameter_line.length());
        return tokens;
    }

    // TOOD: this is a duplicate from RuleChecker::parseCube)
    Cube RuleChecker::parseCube(const std::string &param) {
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

public:
    StatementChecker(KnowledgeBase *kb, Task *task) : kb(kb), task(task) {}
    bool check_subset(const std::string &set1, const std::string &set2) = 0;
    bool check_progression(const std::string &set1, const std::string &set2) = 0;
    bool check_regression(const std::string &set1, const std::string &set2) = 0;
    bool check_is_contained(const Cube &state, const std::string &set) = 0;
    bool check_initial_contained(const std::string &set) = 0;
    bool check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) = 0;
};

#endif // STATEMENTCHECKER_H
