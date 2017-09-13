#ifndef STATEMENTCHECKER_H
#define STATEMENTCHECKER_H

#include "knowledgebase.h"
#include "stateset.h"
#include "task.h"

#include <unordered_map>
#include <fstream>

enum Statement {
    SUBSET,
    EXPLICIT_SUBSET,
    PROGRESSION,
    REGRESSION,
    CONTAINED,
    INITIAL_CONTAINED
};

class StatementChecker
{
protected:
    KnowledgeBase *kb;
    Task *task;
    std::string statementfile;
    std::unordered_map<std::string,Statement> string_to_statement;
    // TODO: this is a duplicate from RuleChecker::determine_parameters
    std::vector<std::string> determine_parameters(const std::string &parameter_line, char delim);
    // TOOD: this is a duplicate from RuleChecker::parseCube)
    Cube parseCube(const std::string &param);

public:
    StatementChecker(KnowledgeBase *kb, Task *task);
    virtual bool check_subset(const std::string &set1, const std::string &set2) = 0;
    virtual bool check_progression(const std::string &set1, const std::string &set2) = 0;
    virtual bool check_regression(const std::string &set1, const std::string &set2) = 0;
    virtual bool check_is_contained(Cube &state, const std::string &set) = 0;
    virtual bool check_initial_contained(const std::string &set) = 0;
    virtual bool check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) = 0;
    void check_statements();
};

#endif // STATEMENTCHECKER_H
