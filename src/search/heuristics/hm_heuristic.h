#ifndef HEURISTICS_HM_HEURISTIC_H
#define HEURISTICS_HM_HEURISTIC_H

#include "../heuristic.h"
#include "../evaluation_context.h"

#include <algorithm>
#include <forward_list>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace options {
class Options;
}

namespace hm_heuristic {
/*
  Haslum's h^m heuristic family ("critical path heuristics").

  This is a very slow implementation and should not be used for
  speed benchmarks.
*/

class HMHeuristic : public Heuristic {
    using Tuple = std::vector<FactPair>;
    // parameters
    const int m;
    const bool has_cond_effects;

    const Tuple goals;

    // h^m table
    std::map<Tuple, int> hm_table;
    bool was_updated;

    bool unsolvability_setup;
    std::vector<std::vector<int>> fact_to_variable;

    std::unordered_map<int,std::forward_list<const Tuple *>> unreachable_tuples;
    int strips_varamount;
    std::string mutexes;
    int mutexamount;

    // auxiliary methods
    void init_hm_table(const Tuple &t);
    void update_hm_table();
    int eval(const Tuple &t) const;
    int update_hm_entry(const Tuple &t, int val);
    void extend_tuple(const Tuple &t, const OperatorProxy &op);

    int check_tuple_in_tuple(const Tuple &tuple, const Tuple &big_tuple) const;

    int get_operator_pre_value(const OperatorProxy &op, int var) const;
    Tuple get_operator_pre(const OperatorProxy &op) const;
    Tuple get_operator_eff(const OperatorProxy &op) const;
    bool contradict_effect_of(const OperatorProxy &op, int var, int val) const;

    void generate_all_tuples();
    void generate_all_tuples_aux(int var, int sz, const Tuple &base);

    void generate_all_partial_tuples(const Tuple &base_tuple,
                                     std::vector<Tuple> &res) const;
    void generate_all_partial_tuples_aux(const Tuple &base_tuple, const Tuple &t, int index,
                                         int sz, std::vector<Tuple> &res) const;

    void dump_table() const;

    void setup_unsolvability_proof();

protected:
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit HMHeuristic(const options::Options &opts);

    virtual bool dead_ends_are_reliable() const;

    virtual void store_deadend_info(EvaluationContext &eval_context) override;
    virtual std::pair<int,int> get_set_and_deadknowledge_id(
            EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) override;
};
}

#endif
