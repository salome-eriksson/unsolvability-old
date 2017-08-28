#ifndef STATEMENTCHECKER_H
#define STATEMENTCHECKER_H

#include "knowledgebase.h"
#include "task.h"

class StatementChecker
{
private:
    KnowledgeBase *kb;
    Task *task;
public:
    StatementChecker(KnowledgeBase *kb, Task *task) : kb(kb), task(task) {}
    bool check_subset(std::string set1, std::string set2) = 0;
    bool check_progression(std::string set1, std::string set2) = 0;
    bool check_regression(std::string set1, std::string set2) = 0;
    bool check_is_contained(Cube state, std::string set) = 0;
    bool check_initial_contained(std::string set) = 0;

    void read_in_sets(std::string filename) = 0;
};

#endif // STATEMENTCHECKER_H
