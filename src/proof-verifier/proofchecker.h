#ifndef PROOFCHECKER_H
#define PROOFCHECKER_H

#include "actionset.h"
#include "knowledge.h"
#include "stateset.h"
#include "task.h"

#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>

enum class ExpressionType {
    BDD,
    HORN,
    DUALHORN,
    EXPLICIT,
    TWOCNF,
    CONSTANT,
    NEGATION,
    INTERSECTION,
    UNION,
    PROGRESSION,
    REGRESSION,
    NONE
};

class ProofChecker
{
private:
    Task task;

    std::deque<std::unique_ptr<StateSet>> formulas;
    std::deque<std::unique_ptr<ActionSet>> actionsets;
    std::deque<std::unique_ptr<Knowledge>> kbentries;
    bool unsolvability_proven;

    // TODO: maybe we can remove this?
    std::map<std::string, ExpressionType> expression_types;
    std::map<std::string, std::function<bool(int, int, std::vector<int> &)>> dead_knowledge_functions;
    std::map<std::string, std::function<bool(int, int, int, std::vector<int> &)>> subset_knowledge_functions;

    template<class T>
    T *get_set_expression(int set_id);
    void add_knowledge(std::unique_ptr<Knowledge> entry, int id);

    /*
     * The return formula serves as a reference which basic formula type is involved.
     * If it is null, then the set is not an intersection of set literals of the same type.
     * If it is a constant formula, then all set variables involved are constant.
     * If it is a concrete type, then all set variables invovled are of this type or constant.
     */
    // TODO: move to StateSetVariable
    StateSetVariable *gather_sets_intersection(StateSet *f,
                                 std::vector<StateSetVariable *> &positive,
                                 std::vector<StateSetVariable *> &negative);
    StateSetVariable *gather_sets_union(StateSet *f,
                           std::vector<StateSetVariable *>&positive,
                           std::vector<StateSetVariable *>&negative);
    StateSetVariable *update_reference_and_check_consistency(StateSetVariable *reference_formula,
                                                       StateSetVariable *tmp, std::string stmt);

    // rules for checking if state sets are dead
    bool check_rule_ed(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    /*bool check_rule_ud(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_sd(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_pg(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_pi(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_rg(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_ri(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);*/

    // rules for ending the proof
    bool check_rule_ci(int conclusion_id, int premise_id);
    bool check_rule_cg(int conclusion_id, int premise_id);


    // rules from basic set theory
    template <class T>
    bool check_rule_ur(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    /*template <class T>
    bool check_rule_ul(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template <class T>
    bool check_rule_ir(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template <class T>
    bool check_rule_il(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template <class T>
    bool check_rule_di(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template <class T>
    bool check_rule_su(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template <class T>
    bool check_rule_si(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template <class T>
    bool check_rule_st(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);*/

    // rules for progression and its relation to regression
    bool check_rule_at(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    /*bool check_rule_au(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_pt(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_pu(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_rp(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_pr(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);*/

    // basic statements
    bool check_statement_B1(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    /*bool check_statement_B2(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_statement_B3(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_statement_B4(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_statement_B5(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);*/

public:
    ProofChecker(std::string &task_file);

    void add_state_set(std::string &line);
    void add_action_set(std::string &line);

    void verify_knowledge(std::string &line);

    bool is_unsolvability_proven();
};

#endif // PROOFCHECKER_H
