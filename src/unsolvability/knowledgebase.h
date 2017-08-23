#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H

#include <unordered_set>

#include "task.h"

class KnowledgeBase
{
private:
    bool unsolvability_proven;
    std::unordered_set<std::string> dead_sets;
    BDD dead_states;
    // represents 2 abstract sets (example: "S;S'" means S[O] \subseteq S \cup S')
    std::unordered_set<std::string> progression;
    // represents 2 abstract sets (example: "S;S'" means R(S,O) \subseteq S \cup S')
    std::unordered_set<std::string> regression;
    // represents 2 abstract sets (example: "S;S'" means S \subseteq S'
    std::unordered_set<std::string> subsets;
    // represents sets containing the initial state
    std::unordered_set<std::string>has_initial;
    // represents sets not containing the initial state
    std::unordered_set<std::string>has_not_initial;
public:
    //TODO: move to more appropriate place
    static std::string EMPTYSET;
    static std::string UNION;
    static std::string INTERSECTION;
    static std::string NEGATION;
    static std::string GOAL;
    static std::string INIT_SET;
    KnowledgeBase();
    bool is_dead_set(const std::string &set);
    bool is_dead_state(const Cube &state);
    bool is_progression(const std::string &set1, const std::string &set2);
    bool is_regression(const std::string &set1, const std::string &set2);
    bool is_subset(const std::string &set1, const std::string &set2);
    // returns true if this relation is in the kb, false otherwise
    bool contains_initial(const std::string &set);
    // returns true if this relation is in the kb, false otherwise
    bool not_contains_initial(const std::string &set);
    bool is_unsolvability_proven();

    void insert_dead_set(const std::string &set);
    void insert_dead_state(const Cube &state);
    void insert_progression(const std::string &set1, const std::string &set2);
    void insert_regression(const std::string &set1, const std::string &set2);
    void insert_subset(const std::string &set1, const std::string &set2);
    void insert_contains_initial(const std::string &set);
    void insert_not_contains_initial(const std::string &set);

    void set_unsolvable_proven();
};

#endif // KNOWLEDGEBASE_H
