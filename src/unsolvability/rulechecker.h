#ifndef RULECHECKER_H
#define RULECHECKER_H

#include "knowledgebase.h"


enum Rules {
    UNION_DEAD = "UD",
    SUBSET_DEAD = "SD",
    STATE_DEAD = "sD",
    PROGRESSION_DEAD ="PD",
    PROGRESSION_NEGATED_DEAD ="Pd",
    REGRESSION_DEAD = "RD",
    REGRESION_NEGATED_DEAD = "Rd",
    PROGRESSION_TO_REGRESSION = "PR",
    REGRESSION_TO_PROGRESSION = "RP",
    UNSOLVABLE_INIT_DEAD = "UI",
    UNSOLVABLE_GOAL_DEAD = "UG",
};

class RuleChecker
{
private:
    KnowledgeBase *kb;
    std::vector<std::string> determine_parameters(const std::string &parameter_line);
    Cube parseCube(const std::string &param);
public:
    RuleChecker();
    bool check_rule_and_insert_into_kb(const std::string &line);
};

#endif // RULECHECKER_H
