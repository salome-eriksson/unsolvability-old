#ifndef STATEMENTCHECKER_H
#define STATEMENTCHECKER_H

#include "knowledgebase.h"
#include "stateset.h"
#include "task.h"

class StatementChecker
{
private:
    KnowledgeBase *kb;
    Task *task;
public:
    StatementChecker(KnowledgeBase *kb, Task *task) : kb(kb), task(task) {}
    bool check_subset(const std::string &set1, const std::string &set2) = 0;
    bool check_progression(const std::string &set1, const std::string &set2) = 0;
    bool check_regression(const std::string &set1, const std::string &set2) = 0;
    bool check_is_contained(const Cube &state, const std::string &set) = 0;
    bool check_initial_contained(const std::string &set) = 0;
    bool check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) = 0;

    void read_in_sets(std::string filename) = 0;
};

#endif // STATEMENTCHECKER_H
