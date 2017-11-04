#ifndef STATEMENTCHECKERBDD_H
#define STATEMENTCHECKERBDD_H

#include <unordered_map>
#include "cuddObj.hh"

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
    void read_in_bdds(std::string filename, std::vector<std::string> &bdd_names);
    void read_in_composite_formulas(std::ifstream &in);
    BDD &get_BDD(const std::string &description, std::vector<BDD> &tmp_bdds);
public:
    StatementCheckerBDD(KnowledgeBase *kb, Task *task, std::ifstream &in);
    bool check_subset(const std::string &set1, const std::string &set2);
    bool check_progression(const std::string &set1, const std::string &set2);
    bool check_regression(const std::string &set1, const std::string &set2);
    bool check_is_contained(Cube &state, const std::string &set);
    bool check_initial_contained(const std::string &set);
    bool check_set_subset_to_stateset(const std::string &set, const StateSet &stateset);
};

#endif // STATEMENTCHECKERBDD_H
