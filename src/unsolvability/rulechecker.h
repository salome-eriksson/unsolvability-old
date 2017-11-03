#ifndef RULECHECKER_H
#define RULECHECKER_H

#include "knowledgebase.h"
#include "task.h"

#include <unordered_map>


enum Rules {
    UNION_DEAD,
    STATEUNION_DEAD,
    SUBSET_DEAD,
    STATE_DEAD,
    PROGRESSION_DEAD,
    PROGRESSION_NEGATED_DEAD,
    REGRESSION_DEAD,
    REGRESION_NEGATED_DEAD,
    PROGRESSION_TO_REGRESSION,
    REGRESSION_TO_PROGRESSION,
    UNSOLVABLE_INIT_DEAD,
    UNSOLVABLE_GOAL_DEAD,
};

class RuleChecker
{
private:
    std::unordered_map<std::string,Rules> string_to_rule;
    KnowledgeBase *kb;
    Task *task;
    std::vector<std::string> determine_parameters(const std::string &parameter_line, char delim);
public:
    RuleChecker(KnowledgeBase *kb, Task *task);
    bool check_rule_and_insert_into_kb(const std::string &line);
    void check_rules_from_file(std::string filename);
};

#endif // RULECHECKER_H
