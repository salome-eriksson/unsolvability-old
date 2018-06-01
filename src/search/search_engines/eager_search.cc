#include "eager_search.h"

#include "../evaluation_context.h"
#include "../globals.h"
#include "../heuristic.h"
#include "../open_list_factory.h"
#include "../option_parser.h"
#include "../pruning_method.h"
#include "../utils/timer.h"

#include "../algorithms/ordered_set.h"
#include "../task_utils/successor_generator.h"

#include "../unsolvability/unsolvabilitymanager.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <set>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <array>
#include <regex>

using namespace std;

namespace eager_search {
EagerSearch::EagerSearch(const Options &opts)
    : SearchEngine(opts),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      use_multi_path_dependence(opts.get<bool>("mpd")),
      generate_certificate(opts.get<bool>("generate_certificate")),
      open_list(opts.get<shared_ptr<OpenListFactory>>("open")->
                create_state_open_list()),
      f_evaluator(opts.get<Evaluator *>("f_eval", nullptr)),
      preferred_operator_heuristics(opts.get_list<Heuristic *>("preferred")),
      pruning_method(opts.get<shared_ptr<PruningMethod>>("pruning")),
      unsolvability_directory(opts.get<std::string>("certificate_directory")) {

    if(generate_certificate) {
        // expand environment variables
        size_t found = unsolvability_directory.find('$');
        while(found != std::string::npos) {
            size_t end = unsolvability_directory.find('/');
            std::string envvar;
            if(end == std::string::npos) {
                envvar = unsolvability_directory.substr(found+1);
            } else {
                envvar = unsolvability_directory.substr(found+1,end-found-1);
            }
            // to upper case
            for(size_t i = 0; i < envvar.size(); i++) {
                envvar.at(i) = toupper(envvar.at(i));
            }
            std::string expanded = std::getenv(envvar.c_str());
            unsolvability_directory.replace(found,envvar.length()+1,expanded);
            found = unsolvability_directory.find('$');
        }
        if(!unsolvability_directory.empty() && !(unsolvability_directory.back() == '/')) {
            unsolvability_directory += "/";
        }
        std::cout << "Generating unsolvability certificate in "
                  << unsolvability_directory << std::endl;
    }
}

void EagerSearch::initialize() {
    cout << "Conducting best first search"
         << (reopen_closed_nodes ? " with" : " without")
         << " reopening closed nodes, (real) bound = " << bound
         << endl;
    if (use_multi_path_dependence)
        cout << "Using multi-path dependence (LM-A*)" << endl;
    assert(open_list);

    set<Evaluator *> evals;
    open_list->get_path_dependent_evaluators(evals);

    // Collect path-dependent evaluators that are used for preferred operators
    // (in case they are not also used in the open list).
    for (Heuristic *heuristic : preferred_operator_heuristics) {
        heuristic->get_path_dependent_evaluators(evals);
    }

    // Collect path-dependent evaluators that are used in the f_evaluator.
    // They are usually also used in the open list and will hence already be
    // included, but we want to be sure.
    if (f_evaluator) {
        f_evaluator->get_path_dependent_evaluators(evals);
    }

    path_dependent_evaluators.assign(evals.begin(), evals.end());

    const GlobalState &initial_state = state_registry.get_initial_state();
    for (Evaluator *evaluator : path_dependent_evaluators) {
        evaluator->notify_initial_state(initial_state);
    }

    // Note: we consider the initial state as reached by a preferred
    // operator.
    EvaluationContext eval_context(initial_state, 0, true, &statistics);

    statistics.inc_evaluated_states();

    if (open_list->is_dead_end(eval_context)) {
        cout << "Initial state is a dead end." << endl;
    } else {
        if (search_progress.check_progress(eval_context))
            print_checkpoint_line(0);
        start_f_value_statistics(eval_context);
        SearchNode node = search_space.get_node(initial_state);
        node.open_initial();

        open_list->insert(eval_context, initial_state.get_id());
    }

    print_initial_h_values(eval_context);

    pruning_method->initialize(task);
}

void EagerSearch::print_checkpoint_line(int g) const {
    cout << "[g=" << g << ", ";
    statistics.print_basic_statistics();
    cout << "]" << endl;
}

void EagerSearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
    pruning_method->print_statistics();
}

SearchStatus EagerSearch::step() {
    pair<SearchNode, bool> n = fetch_next_node();
    if (!n.second) {
        if(generate_certificate) {
            write_unsolvability_certificate();
        }
        return FAILED;
    }
    SearchNode node = n.first;

    GlobalState s = node.get_state();
    if (check_goal_and_set_plan(s))
        return SOLVED;

    vector<OperatorID> applicable_ops;
    g_successor_generator->generate_applicable_ops(s, applicable_ops);

    /*
      TODO: When preferred operators are in use, a preferred operator will be
      considered by the preferred operator queues even when it is pruned.
    */
    pruning_method->prune_operators(s, applicable_ops);

    // This evaluates the expanded state (again) to get preferred ops
    EvaluationContext eval_context(s, node.get_g(), false, &statistics, true);
    ordered_set::OrderedSet<OperatorID> preferred_operators =
        collect_preferred_operators(eval_context, preferred_operator_heuristics);

    for (OperatorID op_id : applicable_ops) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        if ((node.get_real_g() + op.get_cost()) >= bound)
            continue;

        GlobalState succ_state = state_registry.get_successor_state(s, op);
        statistics.inc_generated();
        bool is_preferred = preferred_operators.contains(op_id);

        SearchNode succ_node = search_space.get_node(succ_state);

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end())
            continue;

        // update new path
        if (use_multi_path_dependence || succ_node.is_new()) {
            for (Evaluator *evaluator : path_dependent_evaluators) {
                evaluator->notify_state_transition(s, op_id, succ_state);
            }
        }

        if (succ_node.is_new()) {
            // We have not seen this state before.
            // Evaluate and create a new node.

            // Careful: succ_node.get_g() is not available here yet,
            // hence the stupid computation of succ_g.
            // TODO: Make this less fragile.
            int succ_g = node.get_g() + get_adjusted_cost(op);

            EvaluationContext eval_context(
                succ_state, succ_g, is_preferred, &statistics);
            statistics.inc_evaluated_states();

            if (open_list->is_dead_end(eval_context)) {
                succ_node.mark_as_dead_end();
                statistics.inc_dead_ends();
                continue;
            }
            succ_node.open(node, op);

            open_list->insert(eval_context, succ_state.get_id());
            if (search_progress.check_progress(eval_context)) {
                print_checkpoint_line(succ_node.get_g());
                reward_progress();
            }
        } else if (succ_node.get_g() > node.get_g() + get_adjusted_cost(op)) {
            // We found a new cheapest path to an open or closed state.
            if (reopen_closed_nodes) {
                if (succ_node.is_closed()) {
                    /*
                      TODO: It would be nice if we had a way to test
                      that reopening is expected behaviour, i.e., exit
                      with an error when this is something where
                      reopening should not occur (e.g. A* with a
                      consistent heuristic).
                    */
                    statistics.inc_reopened();
                }
                succ_node.reopen(node, op);

                EvaluationContext eval_context(
                    succ_state, succ_node.get_g(), is_preferred, &statistics);

                /*
                  Note: our old code used to retrieve the h value from
                  the search node here. Our new code recomputes it as
                  necessary, thus avoiding the incredible ugliness of
                  the old "set_evaluator_value" approach, which also
                  did not generalize properly to settings with more
                  than one heuristic.

                  Reopening should not happen all that frequently, so
                  the performance impact of this is hopefully not that
                  large. In the medium term, we want the heuristics to
                  remember heuristic values for states themselves if
                  desired by the user, so that such recomputations
                  will just involve a look-up by the Heuristic object
                  rather than a recomputation of the heuristic value
                  from scratch.
                */
                open_list->insert(eval_context, succ_state.get_id());
            } else {
                // If we do not reopen closed nodes, we just update the parent pointers.
                // Note that this could cause an incompatibility between
                // the g-value and the actual path that is traced back.
                succ_node.update_parent(node, op);
            }
        }
    }

    return IN_PROGRESS;
}

pair<SearchNode, bool> EagerSearch::fetch_next_node() {
    /* TODO: The bulk of this code deals with multi-path dependence,
       which is a bit unfortunate since that is a special case that
       makes the common case look more complicated than it would need
       to be. We could refactor this by implementing multi-path
       dependence as a separate search algorithm that wraps the "usual"
       search algorithm and adds the extra processing in the desired
       places. I think this would lead to much cleaner code. */

    while (true) {
        if (open_list->empty()) {
            cout << "Completely explored state space -- no solution!" << endl;
            // HACK! HACK! we do this because SearchNode has no default/copy constructor
            const GlobalState &initial_state = state_registry.get_initial_state();
            SearchNode dummy_node = search_space.get_node(initial_state);
            return make_pair(dummy_node, false);
        }
        vector<int> last_key_removed;
        StateID id = open_list->remove_min(
            use_multi_path_dependence ? &last_key_removed : nullptr);
        // TODO is there a way we can avoid creating the state here and then
        //      recreate it outside of this function with node.get_state()?
        //      One way would be to store GlobalState objects inside SearchNodes
        //      instead of StateIDs
        GlobalState s = state_registry.lookup_state(id);
        SearchNode node = search_space.get_node(s);

        if (node.is_closed())
            continue;

        if (use_multi_path_dependence) {
            assert(last_key_removed.size() == 2);
            if (node.is_dead_end())
                continue;
            int pushed_h = last_key_removed[1];

            if (!node.is_closed()) {
                EvaluationContext eval_context(
                    node.get_state(), node.get_g(), false, &statistics);

                if (open_list->is_dead_end(eval_context)) {
                    node.mark_as_dead_end();
                    statistics.inc_dead_ends();
                    continue;
                }
                if (pushed_h < eval_context.get_result(path_dependent_evaluators[0]).get_h_value()) {
                    assert(node.is_open());
                    open_list->insert(eval_context, node.get_state_id());
                    continue;
                }
            }
        }

        node.close();
        assert(!node.is_dead_end());
        update_f_value_statistics(node);
        statistics.inc_expanded();
        return make_pair(node, true);
    }
}

void EagerSearch::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    // one of the heuristics finds a state with a new best h value.
    open_list->boost_preferred();
}

void EagerSearch::dump_search_space() const {
    search_space.dump(task_proxy);
}

void EagerSearch::start_f_value_statistics(EvaluationContext &eval_context) {
    if (f_evaluator) {
        int f_value = eval_context.get_heuristic_value(f_evaluator);
        statistics.report_f_value_progress(f_value);
    }
}

/* TODO: HACK! This is very inefficient for simply looking up an h value.
   Also, if h values are not saved it would recompute h for each and every state. */
void EagerSearch::update_f_value_statistics(const SearchNode &node) {
    if (f_evaluator) {
        /*
          TODO: This code doesn't fit the idea of supporting
          an arbitrary f evaluator.
        */
        EvaluationContext eval_context(node.get_state(), node.get_g(), false, &statistics);
        int f_value = eval_context.get_heuristic_value(f_evaluator);
        statistics.report_f_value_progress(f_value);
    }
}

void EagerSearch::write_unsolvability_certificate() {
    double writing_start = utils::g_timer();

    UnsolvabilityManager unsolvmgr(unsolvability_directory, task);
    std::ofstream &certstream = unsolvmgr.get_stream();

    // TODO: asking if the initial node is new seems wrong, but that is
    // how the search handles a dead initial state
    if(search_space.get_node(state_registry.get_initial_state()).is_new()) {
        const GlobalState &init_state = state_registry.get_initial_state();
        EvaluationContext eval_context(init_state,
                                       search_space.get_node(init_state).get_g(),
                                       false, &statistics);
        std::pair<int,int> dead_superset =
                open_list->prove_superset_dead(eval_context, unsolvmgr);
        int knowledge_init_subset = unsolvmgr.get_new_knowledgeid();
        certstream << "k " << knowledge_init_subset << " s " <<  unsolvmgr.get_initsetid()
                   << " " << dead_superset.first << " b1\n";
        int knowledge_init_dead = unsolvmgr.get_new_knowledgeid();
        certstream << "k " << knowledge_init_dead << " d " << unsolvmgr.get_initsetid()
                   << " d3 " << knowledge_init_subset << " " << dead_superset.second << "\n";

        certstream << "k " << unsolvmgr.get_new_knowledgeid() << " u d4 "
                   << knowledge_init_dead << "\n";

        open_list->finish_unsolvability_proof();

        // writing the task file at the end minimizes the chances that both task and cert
        // file are there but the planner could not finish writing them
        unsolvmgr.write_task_file();

        double writing_end = utils::g_timer();
        std::cout << "Time for writing unsolvability certificate: "
                  << writing_end - writing_start << std::endl;
        return;
    }

    struct MergeTreeEntry {
        int de_pos_begin;
        int setid;
        int k_set_dead;
        int depth;
    };

    CuddManager manager(task);
    std::vector<StateID> dead_ends;
    // 2*|DE|-1 for all dead ends, plus the expanded set
    dead_ends.reserve(statistics.get_dead_ends());

    std::vector<MergeTreeEntry> merge_tree;
    merge_tree.resize(log2(statistics.get_dead_ends())+2);
    // mt_pos is the index of the first unused entry of merge_tree
    int mt_pos = 0;

    CuddBDD expanded = CuddBDD(&manager, false);
    CuddBDD dead = CuddBDD(&manager, false);

    for(const StateID id : state_registry) {
        const GlobalState &state = state_registry.lookup_state(id);
        CuddBDD statebdd = CuddBDD(&manager, state);
        if (search_space.get_node(state).is_dead_end()) {

            dead.lor(statebdd);
            dead_ends.push_back(id);

            EvaluationContext eval_context(state,
                                           search_space.get_node(state).get_g(),
                                           false, &statistics);
            std::pair<int,int> dead_superset =
                    open_list->prove_superset_dead(eval_context, unsolvmgr);

            // prove that an explicit set only containing dead end is dead
            int expl_state_setid = unsolvmgr.get_new_setid();
            certstream << "e " << expl_state_setid << " e ";
            unsolvmgr.dump_state(state);
            certstream << " ;\n";
            int k_expl_state_subset = unsolvmgr.get_new_knowledgeid();
            certstream << "k " << k_expl_state_subset << " s " << expl_state_setid << " "
                       << dead_superset.first << " b1\n";
            int k_expl_state_dead = unsolvmgr.get_new_knowledgeid();
            certstream << "k " << k_expl_state_dead << " d " << expl_state_setid
                       << " d3 " << k_expl_state_subset << " " << dead_superset.second << "\n";

            merge_tree[mt_pos].de_pos_begin = dead_ends.size()-1;
            merge_tree[mt_pos].setid = expl_state_setid;
            merge_tree[mt_pos].k_set_dead = k_expl_state_dead;
            merge_tree[mt_pos].depth = 0;
            mt_pos++;

            // merge the last 2 sets to a new one if they have the same depth in the merge tree
            while(mt_pos > 1 && merge_tree[mt_pos-1].depth == merge_tree[mt_pos-2].depth) {
                MergeTreeEntry &mte_left = merge_tree[mt_pos-2];
                MergeTreeEntry &mte_right = merge_tree[mt_pos-1];

                // show that implicit union between the two sets is dead
                int impl_union = unsolvmgr.get_new_setid();
                certstream << "e " << impl_union << " u "
                           << mte_left.setid << " " << mte_right.setid << "\n";
                int k_impl_union_dead = unsolvmgr.get_new_knowledgeid();
                certstream << "k " << k_impl_union_dead << " d " << impl_union
                           << " d2 " << mte_left.k_set_dead << " " << mte_right.k_set_dead << "\n";


                // build new explicit union and show that it is dead
                int union_setid = unsolvmgr.get_new_setid();
                certstream << "e " << union_setid << " e ";
                for(int i = mte_left.de_pos_begin;
                    i < mte_left.de_pos_begin + 2*(1 << mte_left.depth); ++i) {
                    unsolvmgr.dump_state(state_registry.lookup_state(dead_ends[i]));
                    certstream << " ";
                }
                certstream << ";\n";
                int k_union_subset = unsolvmgr.get_new_knowledgeid();
                certstream << "k " << k_union_subset << " s "
                           << union_setid << " " << impl_union << " b2\n";
                int k_union_dead = unsolvmgr.get_new_knowledgeid();
                certstream << "k " << k_union_dead << " d " << union_setid << " d3 "
                           << k_union_subset << " " << k_impl_union_dead << "\n";

                // the left entry represents the merged entry while the right entry will be considered deleted
                mte_left.depth++;
                mte_left.setid = union_setid;
                mte_left.k_set_dead = k_union_dead;
                mt_pos--;
            }

        } else if(search_space.get_node(state).is_closed()) {
            expanded.lor(statebdd);
        }
    }

    std::vector<CuddBDD> bdds;
    std::string filename_search_bdds = unsolvmgr.get_directory() + "search.bdd";
    int de_setid, k_de_dead;

    // no dead ends --> use empty set
    if(dead_ends.size() == 0) {
        de_setid = unsolvmgr.get_emptysetid();
        k_de_dead = unsolvmgr.get_k_empty_dead();
    } else {
        // if the merge tree is not a complete binary tree, we first need to shrink it up to size 1
        // TODO: this is copy paste from above...
        while(mt_pos > 1) {
            MergeTreeEntry &mte_left = merge_tree[mt_pos-2];
            MergeTreeEntry &mte_right = merge_tree[mt_pos-1];

            // show that implicit union between the two sets is dead
            int impl_union = unsolvmgr.get_new_setid();
            certstream << "e " << impl_union << " u "
                       << mte_left.setid << " " << mte_right.setid << "\n";
            int k_impl_union_dead = unsolvmgr.get_new_knowledgeid();
            certstream << "k " << k_impl_union_dead << " d " << impl_union
                       << " d2 " << mte_left.k_set_dead << " " << mte_right.k_set_dead << "\n";

            // build new explicit union and show that it is dead
            int union_setid = unsolvmgr.get_new_setid();
            certstream << "e " << union_setid << " e ";
            for(size_t i = mte_left.de_pos_begin; i < dead_ends.size(); ++i) {
                unsolvmgr.dump_state(state_registry.lookup_state(dead_ends[i]));
                certstream << " ";
            }
            certstream << ";\n";
            int k_union_subset = unsolvmgr.get_new_knowledgeid();
            certstream << "k " << k_union_subset << " s "
                       << union_setid << " " << impl_union << " b2\n";
            int k_union_dead = unsolvmgr.get_new_knowledgeid();
            certstream << "k " << k_union_dead << " d " << union_setid << " d3 "
                       << k_union_subset << " " << k_impl_union_dead << "\n";

            mt_pos--;
            merge_tree[mt_pos-1].depth++;
            merge_tree[mt_pos-1].setid = union_setid;
            merge_tree[mt_pos-1].k_set_dead = k_union_dead;
        }
        bdds.push_back(dead);

        // show that the bdd containing all dead ends is a subset to the explicit set containing all dead ends
        int bdd_dead_setid = unsolvmgr.get_new_setid();
        certstream << "e " << bdd_dead_setid << " b " << filename_search_bdds
                   << " " << bdds.size()-1 << " ;\n";

        int k_bdd_subset_expl = unsolvmgr.get_new_knowledgeid();
        certstream << "k " << k_bdd_subset_expl << " s "
                   << bdd_dead_setid << " " << merge_tree[0].setid << " b1\n";
        int k_bdd_dead = unsolvmgr.get_new_knowledgeid();
        certstream << "k " << k_bdd_dead << " d " << bdd_dead_setid
                   << " d3 " << k_bdd_subset_expl << " " << merge_tree[0].k_set_dead << "\n";

        de_setid = bdd_dead_setid;
        k_de_dead = k_bdd_dead;
    }

    bdds.push_back(expanded);

    // show that expanded states only lead to themselves and dead states
    int expanded_setid = unsolvmgr.get_new_setid();
    certstream << "e " << expanded_setid << " b " << filename_search_bdds << " "
               << bdds.size()-1 << " ;\n";
    int expanded_progression_setid = unsolvmgr.get_new_setid();
    certstream << "e " << expanded_progression_setid << " p " << expanded_setid << "\n";
    int union_expanded_dead = unsolvmgr.get_new_setid();
    certstream << "e " << union_expanded_dead << " u "
               << expanded_setid << " " << de_setid << "\n";

    int k_progression = unsolvmgr.get_new_knowledgeid();
    certstream << "k " << k_progression << " s "
               << expanded_progression_setid << " " << union_expanded_dead << " b4\n";

    int intersection_expanded_goal = unsolvmgr.get_new_setid();
    certstream << "e " << intersection_expanded_goal << " i "
               << expanded_setid << " " << unsolvmgr.get_goalsetid() << "\n";
    int k_exp_goal_empty = unsolvmgr.get_new_knowledgeid();
    certstream << "k " << k_exp_goal_empty << " s "
               << intersection_expanded_goal << " " << unsolvmgr.get_emptysetid() << " b3\n";
    int k_exp_goal_dead = unsolvmgr.get_new_knowledgeid();
    certstream << "k " << k_exp_goal_dead << " d " << intersection_expanded_goal
               << " d3 " << k_exp_goal_empty << " " << unsolvmgr.get_k_empty_dead() << "\n";

    int k_exp_dead = unsolvmgr.get_new_knowledgeid();
    certstream << "k " << k_exp_dead << " d " << expanded_setid << " d6 "
               << k_progression << " " << k_de_dead << " " << k_exp_goal_dead << "\n";

    int k_init_in_exp = unsolvmgr.get_new_knowledgeid();
    certstream << "k " << k_init_in_exp << " s "
               << unsolvmgr.get_initsetid() << " " << expanded_setid << " b1\n";
    int k_init_dead = unsolvmgr.get_new_knowledgeid();
    certstream << "k " << k_init_dead << " d " << unsolvmgr.get_initsetid() << " d3 "
               << k_init_in_exp << " " << k_exp_dead << "\n";
    certstream << "k " << unsolvmgr.get_new_knowledgeid() << " u d4 " << k_init_dead << "\n";

    open_list->finish_unsolvability_proof();

    manager.dumpBDDs(bdds, filename_search_bdds);

    // writing the task file at the end minimizes the chances that both task and cert
    // file are there but the planner could not finish writing them
    unsolvmgr.write_task_file();

    double writing_end = utils::g_timer();
    std::cout << "Time for writing unsolvability certificate: "
              << writing_end - writing_start << std::endl;
}

}
