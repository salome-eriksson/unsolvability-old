#include "rulechecker.h"

RuleChecker::RuleChecker()
{

}


std::vector<std::string> RuleChecker::determine_parameters(std::string parameter_line) {
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


/*
 * Rules in the proof system will look the following:
 * UD:S;S' (means: if S and S' are dead, S \cup S' is dead
 * SD:S;S' (means: if S' is dead and S \subseteq S', then S is dead)
 * PD:S;S' (means: if S[O] \subseteq S \cup S', S' is dead and S \cap S_G is dead, then S is dead)
 * Pd:S;S' (means: if S[O] \subseteq S \cup S', S' is dead and {I} \subseteq S$ then not(S) is dead)
 * Rd:S;S' (means: if R(S,O) \subseteq S \cup S', S' is dead and {I} \subseteq S$ then not(S) is dead)
 * RD:S;S' (means: if R(S,O) \subseteq S \cup S', S' is dead and \cap S_G is dead, then S is dead)
 * UI: (means: if {I} is dead or I is dead, then the task is unsolvable)
 * UG: (means: if S_G is dead, then the task is unsolvable)
 */
bool RuleChecker::check_rule_and_insert_into_kb(std::string line) {
    // TODO: get rule from input
    int pos_colon = s.find(":");
    Rules rule = Rules(line.substr(0, pos_colon));
    std::vector<std::string> param = determine_parameters(line.substr(pos_colon+1));

    switch(rule) {
    case Rules::UNION_DEAD:
        assert(param.size() == 2);
        if(!kb.is_dead_set(param[0]) || !kb.is_dead_set(param[1])) {
            return false;
        } else {
            kb.insert_dead_set(UNION + " " + param[0] + " " + param[1]);
            return true;
        }

    case Rules::SUBSET_DEAD:
        assert(param.size() == 2);
        if(!kb.is_dead_set(param[1]) || !kb.is_subset(param[0], param[1])) {
            return false;
        } else {
            kb.insert_dead_set(param[0]);
            return true;
        }

    case Rules::PROGRESSION_DEAD:
        assert(param.size() == 2);
        if(!kb.is_progression(param[0],param[1]) ||
                !kb.is_dead_set(param[1]) ||
                !kb.is_dead_set(KnowledgeBase::INTERSECTION + " " + param[0] + " " + KnowledgeBase::GOAL)) {
            return false;
        } else {
            kb.insert_dead_set(param[0]);
            return true;
        }

    case Rules::PROGRESSION_NEGATED_DEAD:
        assert(param.size() == 2);
        if(!kb.is_progression(param[0],param[1]) ||
                !kb.is_dead_set(param[1]) ||
                !kb.contains_initial(param[0])) {
            return false;
        } else {
            kb.insert_dead_set(KnowledgeBase::NEGATION + " " + param[0]);
            return true;
        }

    case Rules::REGRESSION_DEAD:
        assert(param.size() == 2);
        if(!kb.is_regression(param[0],param[1]) ||
                !kb.is_dead_set(param[1]) ||
                !kb.not_contains_initial(param[0])) {
            return false;
        } else {
            kb.insert_dead_set(param[0]);
            return true;
        }

    case Rules::REGRESION_NEGATED_DEAD:
        assert(param.size() == 2);
        if(!kb.is_regression(param[0],param[1]) ||
                !kb.is_dead_set(param[1]) ||
                !kb.is_dead_set(KnowledgeBase::INTERSECTION + " " + KnowledgeBase::NEGATION + " " + param[0] + " " + KnowledgeBase::GOAL)) {
            return false;
        } else {
            kb.insert_dead_set(KnowledgeBase::NEGATION + " " + param[0]);
            return true;
        }

    case Rules::UNSOLVABLE_INIT_DEAD:
        //TODO: define initial state somewhere
        if(!kb.is_dead_state(INITIAL_STATE)) {
            return false;
        } else {
            kb.set_unsolvable_proven();
            return true;
        }

    case Rules::UNSOLVABLE_GOAL_DEAD:
        if(!kb.is_dead_set(KnowledgeBase::GOAL)) {
            return false;
        } else {
            kb.set_unsolvable_proven();
            return true;
        }

    default:
        std::cout << "unknown rule";
        return false;
    }
}
