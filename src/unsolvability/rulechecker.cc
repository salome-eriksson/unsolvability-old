#include "rulechecker.h"
#include "global_funcs.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>

RuleChecker::RuleChecker(KnowledgeBase *kb, Task *task)
    : kb(kb), task(task) {
    string_to_rule.insert(std::make_pair("UD",Rules::UNION_DEAD));
    string_to_rule.insert(std::make_pair("uD",Rules::STATEUNION_DEAD));
    string_to_rule.insert(std::make_pair("SD",Rules::SUBSET_DEAD));
    string_to_rule.insert(std::make_pair("sD",Rules::STATE_DEAD));
    string_to_rule.insert(std::make_pair("PD",Rules::PROGRESSION_DEAD));
    string_to_rule.insert(std::make_pair("Pd",Rules::PROGRESSION_NEGATED_DEAD));
    string_to_rule.insert(std::make_pair("RD",Rules::REGRESSION_DEAD));
    string_to_rule.insert(std::make_pair("Rd",Rules::REGRESION_NEGATED_DEAD));
    string_to_rule.insert(std::make_pair("UI",Rules::UNSOLVABLE_INIT_DEAD));
    string_to_rule.insert(std::make_pair("UG",Rules::UNSOLVABLE_GOAL_DEAD));
}

std::vector<std::string> RuleChecker::determine_parameters(const std::string &parameter_line, char delim) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = parameter_line.find(delim, prev);
        if (pos == std::string::npos) {
            pos = parameter_line.length();
        }
        std::string token = parameter_line.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + 1;
    }
    while (pos < parameter_line.length() && prev < parameter_line.length());
    return tokens;
}


/* TODO: how to get from string to enum?
 *
 * Rules in the proof system will look the following:
 * UD:S;S' (means: if S and S' are dead, S \cup S' is dead
 * uD:S (means: if all states in S are dead, then S is dead)
 * SD:S;S' (means: if S' is dead and S \subseteq S', then S is dead)
 * sD:<state>;S (means: if <state> \in S and S is dead, then <state> is dead)
 * PD:S;S' (means: if S[O] \subseteq S \cup S', S' is dead and S \cap S_G is dead, then S is dead)
 * Pd:S;S' (means: if S[O] \subseteq S \cup S', S' is dead and I \in S then not(S) is dead)
 * Rd:S;S' (means: if R(S,O) \subseteq S \cup S', S' is dead and I \not\in S then not(S) is dead)
 * RD:S;S' (means: if R(S,O) \subseteq S \cup S', S' is dead and \cap S_G is dead, then S is dead)
 * UI: (means: if I is dead, then the task is unsolvable)
 * UG: (means: if S_G is dead, then the task is unsolvable)
 */
bool RuleChecker::check_rule_and_insert_into_kb(const std::string &line) {
    int pos_colon = line.find(":");
    assert(string_to_rule.find(line.substr(0,pos_colon)) != string_to_rule.end());
    Rules rule = string_to_rule.find(line.substr(0,pos_colon))->second;
    std::vector<std::string> param = determine_parameters(line.substr(pos_colon+1),';');

    switch(rule) {
    case Rules::UNION_DEAD: {
        assert(param.size() == 2);
        if(!kb->is_dead_set(param[0]) || !kb->is_dead_set(param[1])) {
            return false;
        } else {
            kb->insert_dead_set(KnowledgeBase::UNION + " " + param[0] + " " + param[1]);
            return true;
        }
    }
    case Rules::STATEUNION_DEAD: {
        assert(param.size() == 1);
        if(!kb->all_states_in_set_dead(param[0])) {
            return false;
        } else {
            kb->insert_dead_set(param[0]);
            return true;
        }
    }
    case Rules::SUBSET_DEAD: {
        assert(param.size() == 2);
        if(!kb->is_dead_set(param[1]) || !kb->is_subset(param[0], param[1])) {
            return false;
        } else {
            kb->insert_dead_set(param[0]);
            return true;
        }
    }
    case Rules::STATE_DEAD: {
        assert(param.size() == 2);
        Cube state = parseCube(param[0], task->get_number_of_facts());
        if(!kb->is_contained_in(state,param[1]) || !kb->is_dead_set(param[1])) {
            return false;
        } else {
            kb->insert_dead_state(state);
            return true;
        }
    }
    case Rules::PROGRESSION_DEAD: {
        assert(param.size() == 2);
        if(!kb->is_progression(param[0],param[1]) ||
                !kb->is_dead_set(param[1]) ||
                !kb->is_dead_set(param[0] + " " + special_set_string(SpecialSet::GOALSET) + " " + KnowledgeBase::INTERSECTION)) {
            return false;
        } else {
            kb->insert_dead_set(param[0]);
            return true;
        }
    }
    case Rules::PROGRESSION_NEGATED_DEAD: {
        assert(param.size() == 2);
        if(!kb->is_progression(param[0],param[1]) ||
                !kb->is_dead_set(param[1]) ||
                !kb->contains_initial(param[0])) {
            return false;
        } else {
            kb->insert_dead_set(param[0] + " " + KnowledgeBase::NEGATION);
            return true;
        }
    }
    case Rules::REGRESSION_DEAD: {
        assert(param.size() == 2);
        if(!kb->is_regression(param[0],param[1]) ||
                !kb->is_dead_set(param[1]) ||
                !kb->not_contains_initial(param[0])) {
            return false;
        } else {
            kb->insert_dead_set(param[0]);
            return true;
        }
    }
    case Rules::REGRESION_NEGATED_DEAD: {
        assert(param.size() == 2);
        if(!kb->is_regression(param[0],param[1]) ||
                !kb->is_dead_set(param[1]) ||
                !kb->is_dead_set(param[0] + " " + KnowledgeBase::NEGATION + " " + special_set_string(SpecialSet::GOALSET) + " " + KnowledgeBase::INTERSECTION)) {
            return false;
        } else {
            kb->insert_dead_set(param[0] + " " + KnowledgeBase::NEGATION);
            return true;
        }
    }
    case Rules::UNSOLVABLE_INIT_DEAD: {
        //TODO: copying the initial state cube here is ugly but task->get_initial_state returns a const
        Cube initial_cube = task->get_initial_state();
        if(!kb->is_dead_state(initial_cube)) {
            return false;
        } else {
            kb->set_unsolvability_proven();
            return true;
        }
    }
    case Rules::UNSOLVABLE_GOAL_DEAD: {
        if(!kb->is_dead_set(special_set_string(SpecialSet::GOALSET))) {
            return false;
        } else {
            kb->set_unsolvability_proven();
            return true;
        }
    }
    default: {
        std::cout << "unknown rule";
        return false;
    }
    }
}

void RuleChecker::check_rules_from_file(std::string filename) {
    std::ifstream rules;
    rules.open(filename);
    std::string line;
    while(std::getline(rules,line)) {
        if(!check_rule_and_insert_into_kb(line)) {
            std::cout << "rule not valid: " << line << std::endl;
        }
    }
}
