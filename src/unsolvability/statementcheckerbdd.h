#ifndef STATEMENTCHECKERBDD_H
#define STATEMENTCHECKERBDD_H

#include "unordered_map"

#include "statementchecker.h"

class StatementCheckerBDD : public StatementChecker
{
private:
    Cudd manager;
    std::vector<int> variable_permutation;
    std::vector<int> prime_permutation;
    std::unordered_map<std::string,BDD> bdds;
    BDD* initial_state_bdd;
    BDD* goal_bdd;
    BDD* empty_bdd;
    BDD build_bdd_from_cube(const Cube &cube);
    BDD build_bdd_for_action(const Action &a);
public:
    StatementCheckerBDD(KnowledgeBase *kb, Task *task, const std::vector<int> &variable_permutation);
    bool check_subset(const std::string &set1, const std::string &set2) = 0;
    bool check_progression(const std::string &set1, const std::string &set2) = 0;
    bool check_regression(const std::string &set1, const std::string &set2) = 0;
    bool check_is_contained(const Cube &state, const std::string &set) = 0;
    bool check_initial_contained(const std::string &set) = 0;
    bool check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) = 0;

    void read_in_sets(std::string filename);
};

#endif // STATEMENTCHECKERBDD_H
