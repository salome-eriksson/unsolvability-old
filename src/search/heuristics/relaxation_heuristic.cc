#include "relaxation_heuristic.h"

#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <sstream>

using namespace std;

namespace relaxation_heuristic {
Proposition::Proposition()
    : cost(-1),
      reached_by(NO_OP),
      is_goal(false),
      marked(false),
      num_precondition_occurences(-1) {
}


UnaryOperator::UnaryOperator(
    int num_preconditions, array_pool::ArrayPoolIndex preconditions,
    PropID effect, int operator_no, int base_cost)
    : effect(effect),
      base_cost(base_cost),
      num_preconditions(num_preconditions),
      preconditions(preconditions),
      operator_no(operator_no) {
}


// construction and destruction
// TODO: unsolv_subsumption_check is currently hacked into max_heuristic...
RelaxationHeuristic::RelaxationHeuristic(const options::Options &opts)
    : Heuristic(opts), unsolv_subsumption_check(false),
      unsolvability_setup(false) {
    // Build propositions.
    propositions.resize(task_properties::get_num_facts(task_proxy));

    // Build proposition offsets.
    VariablesProxy variables = task_proxy.get_variables();
    proposition_offsets.reserve(variables.size());
    PropID offset = 0;
    for (VariableProxy var : variables) {
        proposition_offsets.push_back(offset);
        offset += var.get_domain_size();
    }
    assert(offset == static_cast<int>(propositions.size()));

    // Build goal propositions.
    GoalsProxy goals = task_proxy.get_goals();
    goal_propositions.reserve(goals.size());
    for (FactProxy goal : goals) {
        PropID prop_id = get_prop_id(goal);
        propositions[prop_id].is_goal = true;
        goal_propositions.push_back(prop_id);
    }

    // Build unary operators for operators and axioms.
    unary_operators.reserve(
        task_properties::get_num_total_effects(task_proxy));
    for (OperatorProxy op : task_proxy.get_operators())
        build_unary_operators(op);
    for (OperatorProxy axiom : task_proxy.get_axioms())
        build_unary_operators(axiom);

    // Simplify unary operators.
    utils::Timer simplify_timer;
    simplify();
    cout << "time to simplify: " << simplify_timer << endl;

    // Cross-reference unary operators.
    vector<vector<OpID>> precondition_of_vectors(propositions.size());

    int num_unary_ops = unary_operators.size();
    for (OpID op_id = 0; op_id < num_unary_ops; ++op_id) {
        for (PropID precond : get_preconditions(op_id))
            precondition_of_vectors[precond].push_back(op_id);
    }

    int num_propositions = propositions.size();
    for (PropID prop_id = 0; prop_id < num_propositions; ++prop_id) {
        auto precondition_of_vec = move(precondition_of_vectors[prop_id]);
        propositions[prop_id].precondition_of =
            precondition_of_pool.append(precondition_of_vec);
        propositions[prop_id].num_precondition_occurences = precondition_of_vec.size();
    }
}

bool RelaxationHeuristic::dead_ends_are_reliable() const {
    return !task_properties::has_axioms(task_proxy);
}

PropID RelaxationHeuristic::get_prop_id(int var, int value) const {
    return proposition_offsets[var] + value;
}

PropID RelaxationHeuristic::get_prop_id(const FactProxy &fact) const {
    return get_prop_id(fact.get_variable().get_id(), fact.get_value());
}

const Proposition *RelaxationHeuristic::get_proposition(
    int var, int value) const {
    return &propositions[get_prop_id(var, value)];
}

Proposition *RelaxationHeuristic::get_proposition(int var, int value) {
    return &propositions[get_prop_id(var, value)];
}

Proposition *RelaxationHeuristic::get_proposition(const FactProxy &fact) {
    return get_proposition(fact.get_variable().get_id(), fact.get_value());
}

void RelaxationHeuristic::build_unary_operators(const OperatorProxy &op) {
    int op_no = op.is_axiom() ? -1 : op.get_id();
    int base_cost = op.get_cost();
    vector<PropID> precondition_props;
    PreconditionsProxy preconditions = op.get_preconditions();
    precondition_props.reserve(preconditions.size());
    for (FactProxy precondition : preconditions) {
        precondition_props.push_back(get_prop_id(precondition));
    }
    for (EffectProxy effect : op.get_effects()) {
        PropID effect_prop = get_prop_id(effect.get_fact());
        EffectConditionsProxy eff_conds = effect.get_conditions();
        precondition_props.reserve(preconditions.size() + eff_conds.size());
        for (FactProxy eff_cond : eff_conds) {
            precondition_props.push_back(get_prop_id(eff_cond));
        }

        // The sort-unique can eventually go away. See issue497.
        vector<PropID> preconditions_copy(precondition_props);
        utils::sort_unique(preconditions_copy);
        array_pool::ArrayPoolIndex precond_index =
            preconditions_pool.append(preconditions_copy);
        unary_operators.emplace_back(
            preconditions_copy.size(), precond_index, effect_prop,
            op_no, base_cost);
        precondition_props.erase(precondition_props.end() - eff_conds.size(), precondition_props.end());
    }
}

void RelaxationHeuristic::simplify() {
    /*
      Remove dominated unary operators, including duplicates.

      Unary operators with more than MAX_PRECONDITIONS_TO_TEST
      preconditions are (mostly; see code comments below for details)
      ignored because we cannot handle them efficiently. This is
      obviously an inelegant solution.

      Apart from this restriction, operator o1 dominates operator o2 if:
      1. eff(o1) = eff(o2), and
      2. pre(o1) is a (not necessarily strict) subset of pre(o2), and
      3. cost(o1) <= cost(o2), and either
      4a. At least one of 2. and 3. is strict, or
      4b. id(o1) < id(o2).
      (Here, "id" is the position in the unary_operators vector.)

      This defines a strict partial order.
    */
#ifndef NDEBUG
    int num_ops = unary_operators.size();
    for (OpID op_id = 0; op_id < num_ops; ++op_id)
        assert(utils::is_sorted_unique(get_preconditions_vector(op_id)));
#endif

    const int MAX_PRECONDITIONS_TO_TEST = 5;

    cout << "Simplifying " << unary_operators.size() << " unary operators..." << flush;

    /*
      First, we create a map that maps the preconditions and effect
      ("key") of each operator to its cost and index ("value").
      If multiple operators have the same key, the one with lowest
      cost wins. If this still results in a tie, the one with lowest
      index wins. These rules can be tested with a lexicographical
      comparison of the value.

      Note that for operators sharing the same preconditions and
      effect, our dominance relationship above is actually a strict
      *total* order (order by cost, then by id).

      For each key present in the data, the map stores the dominating
      element in this total order.
    */
    using Key = pair<vector<PropID>, PropID>;
    using Value = pair<int, OpID>;
    using Map = utils::HashMap<Key, Value>;
    Map unary_operator_index;
    unary_operator_index.reserve(unary_operators.size());

    for (size_t op_no = 0; op_no < unary_operators.size(); ++op_no) {
        const UnaryOperator &op = unary_operators[op_no];
        /*
          Note: we consider operators with more than
          MAX_PRECONDITIONS_TO_TEST preconditions here because we can
          still filter out "exact matches" for these, i.e., the first
          test in `is_dominated`.
        */

        Key key(get_preconditions_vector(op_no), op.effect);
        Value value(op.base_cost, op_no);
        auto inserted = unary_operator_index.insert(
            make_pair(move(key), value));
        if (!inserted.second) {
            // We already had an element with this key; check its cost.
            Map::iterator iter = inserted.first;
            Value old_value = iter->second;
            if (value < old_value)
                iter->second = value;
        }
    }

    /*
      `dominating_key` is conceptually a local variable of `is_dominated`.
      We declare it outside to reduce vector allocation overhead.
    */
    Key dominating_key;

    /*
      is_dominated: test if a given operator is dominated by an
      operator in the map.
    */
    auto is_dominated = [&](const UnaryOperator &op) {
            /*
              Check all possible subsets X of pre(op) to see if there is a
              dominating operator with preconditions X represented in the
              map.
            */

            OpID op_id = get_op_id(op);
            int cost = op.base_cost;

            const vector<PropID> precondition = get_preconditions_vector(op_id);

            /*
              We handle the case X = pre(op) specially for efficiency and
              to ensure that an operator is not considered to be dominated
              by itself.

              From the discussion above that operators with the same
              precondition and effect are actually totally ordered, it is
              enough to test here whether looking up the key of op in the
              map results in an entry including op itself.
            */
            if (unary_operator_index[make_pair(precondition, op.effect)].second != op_id)
                return true;

            /*
              We now handle all cases where X is a strict subset of pre(op).
              Our map lookup ensures conditions 1. and 2., and because X is
              a strict subset, we also have 4a (which means we don't need 4b).
              So it only remains to check 3 for all hits.
            */
            if (op.num_preconditions > MAX_PRECONDITIONS_TO_TEST) {
                /*
                  The runtime of the following code grows exponentially
                  with the number of preconditions.
                */
                return false;
            }

            vector<PropID> &dominating_precondition = dominating_key.first;
            dominating_key.second = op.effect;

            // We subtract "- 1" to generate all *strict* subsets of precondition.
            int powerset_size = (1 << precondition.size()) - 1;
            for (int mask = 0; mask < powerset_size; ++mask) {
                dominating_precondition.clear();
                for (size_t i = 0; i < precondition.size(); ++i)
                    if (mask & (1 << i))
                        dominating_precondition.push_back(precondition[i]);
                Map::iterator found = unary_operator_index.find(dominating_key);
                if (found != unary_operator_index.end()) {
                    Value dominator_value = found->second;
                    int dominator_cost = dominator_value.first;
                    if (dominator_cost <= cost)
                        return true;
                }
            }
            return false;
        };

    unary_operators.erase(
        remove_if(
            unary_operators.begin(),
            unary_operators.end(),
            is_dominated),
        unary_operators.end());

    cout << " done! [" << unary_operators.size() << " unary operators]" << endl;
}

// CARE: we assume the heuristic has just been calculated for this state
std::pair<bool,int> RelaxationHeuristic::get_bdd_for_state(const GlobalState &state) {
    auto it = state_to_bddindex.find(state.get_id().get_value());
    if (it != state_to_bddindex.end()) {
        return std::make_pair(true, it->second);
    }
    CuddBDD statebdd(cudd_manager, state);
    if(unsolv_subsumption_check) {
        for(size_t i = 0; i < bdds.size(); ++i) {
            if(statebdd.isSubsetOf(bdds[i])) {
                return std::make_pair(true,i);
            }
        }
    }

    std::vector<std::pair<int,int>> pos_vars;
    std::vector<std::pair<int,int>> neg_vars;
    for(size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        for(int j = 0; j < task_proxy.get_variables()[i].get_domain_size(); ++j) {
            if(propositions[proposition_offsets[i]+j].cost == -1) {
                neg_vars.push_back(std::make_pair(i,j));
            }
        }
    }
    bdds.push_back(CuddBDD(cudd_manager, pos_vars,neg_vars));
    state_to_bddindex[state.get_id().get_value()] = bdds.size()-1;
    return std::make_pair(false,bdds.size()-1);
}

int RelaxationHeuristic::create_subcertificate(EvaluationContext &eval_context) {
    if(!unsolvability_setup) {
        cudd_manager = new CuddManager(task);
        unsolvability_setup = true;
    }
    std::pair<bool,int> get_bdd = get_bdd_for_state(eval_context.get_state());
    bool bdd_already_seen = get_bdd.first;
    // we have used this bdd for another dead end already, use this stateid
    if(bdd_already_seen) {
        int bddindex = get_bdd.second;
        return bdd_to_stateid[bddindex];
    }
    int stateid = eval_context.get_state().get_id().get_value();
    bdd_to_stateid.push_back(stateid);
    return stateid;
}

void RelaxationHeuristic::write_subcertificates(const string &filename) {
    if(!bdds.empty()) {
        cudd_manager->dumpBDDs_certificate(bdds, bdd_to_stateid, filename);
    } else {
        std::ofstream cert_stream;
        cert_stream.open(filename);
        cert_stream.close();
    }
}

void RelaxationHeuristic::store_deadend_info(EvaluationContext &eval_context) {
    if(!unsolvability_setup) {
        cudd_manager = new CuddManager(task);
        unsolvability_setup = true;
    }

    int bddindex = get_bdd_for_state(eval_context.get_state()).second;
    state_to_bddindex.insert({eval_context.get_state().get_id().get_value(), bddindex});
}

std::pair<int,int> RelaxationHeuristic::get_set_and_deadknowledge_id(
        EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) {
    if (set_and_knowledge_ids.empty()) {
        std::stringstream ss;
        ss << unsolvmanager.get_directory() << this << ".bdd";
        bdd_filename = ss.str();
        set_and_knowledge_ids.resize(bdds.size(), {-1,-1});
    }
    int bddindex = state_to_bddindex[eval_context.get_state().get_id().get_value()];
    assert(bddindex >= 0);
    std::pair<int,int> &ids = set_and_knowledge_ids[bddindex];

    if(ids.first == -1) {
        int setid = unsolvmanager.get_new_setid();

        std::ofstream &certstream = unsolvmanager.get_stream();
        certstream << "e " << setid << " b " << bdd_filename << " " << bddindex << " ;\n";
        int progid = unsolvmanager.get_new_setid();
        certstream << "e " << progid << " p " << setid << " 0" << "\n";
        int union_set_empty = unsolvmanager.get_new_setid();;
        certstream << "e " << union_set_empty << " u "
                   << setid << " " << unsolvmanager.get_emptysetid() << "\n";

        int k_prog = unsolvmanager.get_new_knowledgeid();
        certstream << "k " << k_prog << " s " << progid << " " << union_set_empty << " b2\n";

        int set_and_goal = unsolvmanager.get_new_setid();
        certstream << "e " << set_and_goal << " i "
                   << setid << " " << unsolvmanager.get_goalsetid() << "\n";
        int k_set_and_goal_empty = unsolvmanager.get_new_knowledgeid();
        certstream << "k " << k_set_and_goal_empty << " s "
                   << set_and_goal << " " << unsolvmanager.get_emptysetid() << " b1\n";
        int k_set_and_goal_dead = unsolvmanager.get_new_knowledgeid();
        certstream << "k " << k_set_and_goal_dead << " d " << set_and_goal
                   << " d3 " << k_set_and_goal_empty << " " << unsolvmanager.get_k_empty_dead() << "\n";

        int k_set_dead = unsolvmanager.get_new_knowledgeid();
        certstream << "k " << k_set_dead << " d " << setid << " d6 " << k_prog << " "
                   << unsolvmanager.get_k_empty_dead() << " " << k_set_and_goal_dead << "\n";

        ids.first = setid;
        ids.second = k_set_dead;
    }
    return ids;
}

void RelaxationHeuristic::finish_unsolvability_proof() {
    if(!bdds.empty()) {
        cudd_manager->dumpBDDs(bdds, bdd_filename);
    }
}
}
