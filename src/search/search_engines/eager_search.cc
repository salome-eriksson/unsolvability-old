#include "eager_search.h"

#include "search_common.h"

#include "../evaluation_context.h"
#include "../globals.h"
#include "../heuristic.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../pruning_method.h"
#include "../successor_generator.h"
#include "../utils/timer.h"

#include "../algorithms/ordered_set.h"

#include "../open_lists/open_list_factory.h"

#include "../unsolvability/unsolvabilitymanager.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <set>
#include <fstream>
#include <stdlib.h>
#include <math.h>

using namespace std;
static inline int get_op_index(const GlobalOperator *op) {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < static_cast<int>(g_operators.size()));
    return op_index;
}

namespace eager_search {
EagerSearch::EagerSearch(const Options &opts)
    : SearchEngine(opts),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      use_multi_path_dependence(opts.get<bool>("mpd")),
      open_list(opts.get<shared_ptr<OpenListFactory>>("open")->
                create_state_open_list()),
      f_evaluator(opts.get<ScalarEvaluator *>("f_eval", nullptr)),
      preferred_operator_heuristics(opts.get_list<Heuristic *>("preferred")),
      pruning_method(opts.get<shared_ptr<PruningMethod>>("pruning")) {
}

void EagerSearch::initialize() {
    cout << "Conducting best first search"
         << (reopen_closed_nodes ? " with" : " without")
         << " reopening closed nodes, (real) bound = " << bound
         << endl;
    if (use_multi_path_dependence)
        cout << "Using multi-path dependence (LM-A*)" << endl;
    assert(open_list);

    set<Heuristic *> hset;
    open_list->get_involved_heuristics(hset);

    // add heuristics that are used for preferred operators (in case they are
    // not also used in the open list)
    hset.insert(preferred_operator_heuristics.begin(),
                preferred_operator_heuristics.end());

    // add heuristics that are used in the f_evaluator. They are usually also
    // used in the open list and hence already be included, but we want to be
    // sure.
    if (f_evaluator) {
        f_evaluator->get_involved_heuristics(hset);
    }

    heuristics.assign(hset.begin(), hset.end());
    assert(!heuristics.empty());

    const GlobalState &initial_state = state_registry.get_initial_state();
    for (Heuristic *heuristic : heuristics) {
        heuristic->notify_initial_state(initial_state);
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
        write_unsolvability_certificate();
        return FAILED;
    }
    SearchNode node = n.first;

    GlobalState s = node.get_state();
    if (check_goal_and_set_plan(s))
        return SOLVED;

    vector<const GlobalOperator *> applicable_ops;
    g_successor_generator->generate_applicable_ops(s, applicable_ops);

    /*
      TODO: When preferred operators are in use, a preferred operator will be
      considered by the preferred operator queues even when it is pruned.
    */
    pruning_method->prune_operators(s, applicable_ops);

    // This evaluates the expanded state (again) to get preferred ops
    EvaluationContext eval_context(s, node.get_g(), false, &statistics, true);
    algorithms::OrderedSet<const GlobalOperator *> preferred_operators =
        collect_preferred_operators(eval_context, preferred_operator_heuristics);

    for (const GlobalOperator *op : applicable_ops) {
        if ((node.get_real_g() + op->get_cost()) >= bound)
            continue;

        GlobalState succ_state = state_registry.get_successor_state(s, *op);
        statistics.inc_generated();
        bool is_preferred = preferred_operators.contains(op);

        SearchNode succ_node = search_space.get_node(succ_state);

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end())
            continue;

        // update new path
        if (use_multi_path_dependence || succ_node.is_new()) {
            /*
              Note: we must call notify_state_transition for each heuristic, so
              don't break out of the for loop early.
            */
            for (Heuristic *heuristic : heuristics) {
                heuristic->notify_state_transition(s, *op, succ_state);
            }
        }

        if (succ_node.is_new()) {
            // We have not seen this state before.
            // Evaluate and create a new node.

            // Careful: succ_node.get_g() is not available here yet,
            // hence the stupid computation of succ_g.
            // TODO: Make this less fragile.
            int succ_g = node.get_g() + get_adjusted_cost(*op);

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
        } else if (succ_node.get_g() > node.get_g() + get_adjusted_cost(*op)) {
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
                if (pushed_h < eval_context.get_result(heuristics[0]).get_h_value()) {
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
    search_space.dump();
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

/* TODO: merge this into SearchEngine::add_options_to_parser when all search
         engines support pruning. */
void add_pruning_option(OptionParser &parser) {
    parser.add_option<shared_ptr<PruningMethod>>(
        "pruning",
        "Pruning methods can prune or reorder the set of applicable operators in "
        "each state and thereby influence the number and order of successor states "
        "that are considered.",
        "null()");
}


void EagerSearch::write_unsolvability_certificate() {
    double writing_start = utils::g_timer();

    UnsolvabilityManager &unsolvmgr = UnsolvabilityManager::getInstance();
    std::ofstream &certstream = unsolvmgr.get_stream();
    unsolvmgr.write_task_file();

    // TODO: asking if the initial node is new seems wrong, but that is
    // how the search handles a dead initial state
    if(search_space.get_node(state_registry.get_initial_state()).is_new()) {
        const GlobalState &init_state = state_registry.get_initial_state();
        EvaluationContext eval_context(init_state,nullptr,false);
        Heuristic *h = nullptr;
        for(size_t i = 0; i < heuristics.size(); ++i) {
            if(eval_context.is_heuristic_infinite(heuristics[i])) {
                h = heuristics[i];
                heuristics[i]->setup_unsolvability_proof();
                break;
            }
        }
        assert(h != nullptr);
        std::pair<int,int> superset = h->prove_superset_dead(init_state);
        int knowledge_init_subset = unsolvmgr.get_new_knowledgeid();
        certstream << "k " << knowledge_init_subset << " s " <<  unsolvmgr.get_initsetid()
                   << " " << superset.first << " d1\n";
        int knowledge_init_dead = unsolvmgr.get_new_knowledgeid();
        certstream << "k " << knowledge_init_dead << " d " << unsolvmgr.get_initsetid()
                   << " d3 " << knowledge_init_subset << " " << superset.second << "\n";

        certstream << "k " << unsolvmgr.get_new_knowledgeid() << " u d4 "
                   << knowledge_init_dead << "\n";

        double writing_end = utils::g_timer();
        std::cout << "Time for writing unsolvability certificate: "
                  << writing_end - writing_start << std::endl;
        return;
    }


    CuddManager manager;
    std::vector<CuddBDD> bdds;
    // 2*|DE|-1 for all dead ends, plus the expanded set
    bdds.reserve(statistics.get_dead_ends()*2);

    int old_union_setid = -1;
    CuddBDD *old_union_bdd = nullptr;
    int k_old_union_dead = -1;
    std::string filename_search_bdds = unsolvmgr.get_directory() + "search.bdd";

    CuddBDD expanded = CuddBDD(&manager, false);

    std::vector<bool> heuristic_used(heuristics.size(), false);

    for(const StateID id : state_registry) {
        const GlobalState &state = state_registry.lookup_state(id);
        CuddBDD statebdd = CuddBDD(&manager, state);
        if (search_space.get_node(state).is_dead_end()) {
            // find the heuristic that evaluated the state to be a dead end
            EvaluationContext eval_context(state,nullptr,false);
            Heuristic *h = nullptr;
            for(size_t i = 0; i < heuristics.size(); ++i) {
                if(eval_context.is_heuristic_infinite(heuristics[i])) {
                    h = heuristics[i];
                    if(!heuristic_used[i]) {
                        heuristics[i]->setup_unsolvability_proof();
                        heuristic_used[i] = true;
                    }
                    break;
                }
            }
            assert(h != nullptr);
            std::pair<int,int> dead_superset = h->prove_superset_dead(state);

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


            bdds.push_back(statebdd);
            CuddBDD *dead_state_bdd = &(bdds[bdds.size()-1]);

            int bdd_state_setid = unsolvmgr.get_new_setid();
            certstream << "e " << bdd_state_setid << " b " << filename_search_bdds << " "
                       << bdds.size()-1 << " ;\n";
            int k_bdd_state_subset = unsolvmgr.get_new_knowledgeid();
            certstream << "k " << k_bdd_state_subset << " s " << bdd_state_setid << " "
                       << expl_state_setid << " b1\n";
            int k_bdd_state_dead = unsolvmgr.get_new_knowledgeid();
            certstream << "k " << k_bdd_state_dead << " d " << bdd_state_setid
                       << " d3 " << k_bdd_state_subset << " " << k_expl_state_dead << "\n";

            // TODO: can we do this case of first dead end nicer?
            if(!old_union_bdd) {
                old_union_bdd = dead_state_bdd;
                old_union_setid = bdd_state_setid;
                k_old_union_dead = k_bdd_state_dead;
            } else {
                // show that implicit union (between new dead state and old dead ends) is dead
                int impl_union = unsolvmgr.get_new_setid();
                certstream << "e " << impl_union << " u "
                           << bdd_state_setid << " " << old_union_setid << "\n";
                int k_impl_union_dead = unsolvmgr.get_new_knowledgeid();
                certstream << "k " << k_impl_union_dead << " d " << impl_union
                           << " d2 " << k_bdd_state_dead << " " << k_old_union_dead << "\n";

                // build new explicit union and show that it is dead
                bdds.push_back(CuddBDD(*old_union_bdd));
                CuddBDD *new_union_bdd = &(bdds[bdds.size()-1]);
                new_union_bdd->lor(*dead_state_bdd);
                int new_union_setid = unsolvmgr.get_new_setid();
                certstream << "e " << new_union_setid << " b " << filename_search_bdds << " "
                           << bdds.size()-1 << " ;\n";
                int k_new_union_subset = unsolvmgr.get_new_knowledgeid();
                certstream << "k " << k_new_union_subset << " s "
                           << new_union_setid << " " << impl_union << " b2\n";
                int k_new_union_dead = unsolvmgr.get_new_knowledgeid();
                certstream << "k " << k_new_union_dead << " d " << new_union_setid << " d3 "
                           << k_new_union_subset << " " << k_impl_union_dead << "\n";

                old_union_bdd = new_union_bdd;
                old_union_setid = new_union_setid;
                k_old_union_dead = k_new_union_dead;
            }

        } else if(search_space.get_node(state).is_closed()) {
            expanded.lor(statebdd);
        }
    }

    // old_union now contains all dead ends and expanded all expanded states
    bdds.push_back(expanded);
    int expanded_setid = unsolvmgr.get_new_setid();
    certstream << "e " << expanded_setid << " b " << filename_search_bdds << " "
               << bdds.size()-1 << " ;\n";
    int expanded_progression_setid = unsolvmgr.get_new_setid();
    certstream << "e " << expanded_progression_setid << " p " << expanded_setid;
    int union_expanded_dead = unsolvmgr.get_new_setid();
    certstream << "e " << union_expanded_dead << " u "
               << expanded_setid << " " << old_union_setid << "\n";

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
               << k_progression << k_old_union_dead << k_exp_goal_dead << "\n";

    int k_init_in_exp = unsolvmgr.get_new_knowledgeid();
    certstream << "k " << k_init_in_exp << " s "
               << unsolvmgr.get_initsetid() << " " << expanded_setid << " b1\n";
    int k_init_dead = unsolvmgr.get_new_knowledgeid();
    certstream << "k " << k_init_dead << " d " << unsolvmgr.get_initsetid() << " d3 "
               << k_init_in_exp << " " << k_exp_dead << "\n";
    certstream << "k " << unsolvmgr.get_new_knowledgeid() << " u d4 " << k_init_dead << "\n";

    for(size_t i = 0; i < heuristics.size(); ++i) {
        if(heuristic_used[i]) {
            heuristics[i]->finish_unsolvability_proof();
        }
    }

    manager.dumpBDDs(bdds, filename_search_bdds);

    double writing_end = utils::g_timer();
    std::cout << "Time for writing unsolvability certificate: "
              << writing_end - writing_start << std::endl;
}

static SearchEngine *_parse(OptionParser &parser) {
    parser.document_synopsis("Eager best-first search", "");

    parser.add_option<shared_ptr<OpenListFactory>>("open", "open list");
    parser.add_option<bool>("reopen_closed",
                            "reopen closed nodes", "false");
    parser.add_option<ScalarEvaluator *>(
        "f_eval",
        "set evaluator for jump statistics. "
        "(Optional; if no evaluator is used, jump statistics will not be displayed.)",
        OptionParser::NONE);
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");

    add_pruning_option(parser);
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    EagerSearch *engine = nullptr;
    if (!parser.dry_run()) {
        opts.set<bool>("mpd", false);
        engine = new EagerSearch(opts);
    }

    return engine;
}

static SearchEngine *_parse_astar(OptionParser &parser) {
    parser.document_synopsis(
        "A* search (eager)",
        "A* is a special case of eager best first search that uses g+h "
        "as f-function. "
        "We break ties using the evaluator. Closed nodes are re-opened.");
    parser.document_note(
        "mpd option",
        "This option is currently only present for the A* algorithm and not "
        "for the more general eager search, "
        "because the current implementation of multi-path depedence "
        "does not support general open lists.");
    parser.document_note(
        "Equivalent statements using general eager search",
        "\n```\n--search astar(evaluator)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h=evaluator\n"
        "--search eager(tiebreaking([sum([g(), h]), h], unsafe_pruning=false),\n"
        "               reopen_closed=true, f_eval=sum([g(), h]))\n"
        "```\n", true);
    parser.add_option<ScalarEvaluator *>("eval", "evaluator for h-value");
    parser.add_option<bool>("mpd",
                            "use multi-path dependence (LM-A*)", "false");

    add_pruning_option(parser);
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    EagerSearch *engine = nullptr;
    if (!parser.dry_run()) {
        auto temp = search_common::create_astar_open_list_factory_and_f_eval(opts);
        opts.set("open", temp.first);
        opts.set("f_eval", temp.second);
        opts.set("reopen_closed", true);
        vector<Heuristic *> preferred_list;
        opts.set("preferred", preferred_list);
        engine = new EagerSearch(opts);
    }

    return engine;
}

static SearchEngine *_parse_greedy(OptionParser &parser) {
    parser.document_synopsis("Greedy search (eager)", "");
    parser.document_note(
        "Open list",
        "In most cases, eager greedy best first search uses "
        "an alternation open list with one queue for each evaluator. "
        "If preferred operator heuristics are used, it adds an extra queue "
        "for each of these evaluators that includes only the nodes that "
        "are generated with a preferred operator. "
        "If only one evaluator and no preferred operator heuristic is used, "
        "the search does not use an alternation open list but a "
        "standard open list with only one queue.");
    parser.document_note(
        "Closed nodes",
        "Closed node are not re-opened");
    parser.document_note(
        "Equivalent statements using general eager search",
        "\n```\n--heuristic h2=eval2\n"
        "--search eager_greedy([eval1, h2], preferred=h2, boost=100)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h1=eval1 --heuristic h2=eval2\n"
        "--search eager(alt([single(h1), single(h1, pref_only=true), single(h2), \n"
        "                    single(h2, pref_only=true)], boost=100),\n"
        "               preferred=h2)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search eager_greedy([eval1, eval2])\n```\n"
        "is equivalent to\n"
        "```\n--search eager(alt([single(eval1), single(eval2)]))\n```\n"
        "------------------------------------------------------------\n"
        "```\n--heuristic h1=eval1\n"
        "--search eager_greedy(h1, preferred=h1)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h1=eval1\n"
        "--search eager(alt([single(h1), single(h1, pref_only=true)]),\n"
        "               preferred=h1)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search eager_greedy(eval1)\n```\n"
        "is equivalent to\n"
        "```\n--search eager(single(eval1))\n```\n", true);

    parser.add_list_option<ScalarEvaluator *>("evals", "scalar evaluators");
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");
    parser.add_option<int>(
        "boost",
        "boost value for preferred operator open lists", "0");

    add_pruning_option(parser);
    SearchEngine::add_options_to_parser(parser);

    Options opts = parser.parse();
    opts.verify_list_non_empty<ScalarEvaluator *>("evals");

    EagerSearch *engine = nullptr;
    if (!parser.dry_run()) {
        opts.set("open", search_common::create_greedy_open_list_factory(opts));
        opts.set("reopen_closed", false);
        opts.set("mpd", false);
        ScalarEvaluator *evaluator = nullptr;
        opts.set("f_eval", evaluator);
        engine = new EagerSearch(opts);
    }
    return engine;
}

static Plugin<SearchEngine> _plugin("eager", _parse);
static Plugin<SearchEngine> _plugin_astar("astar", _parse_astar);
static Plugin<SearchEngine> _plugin_greedy("eager_greedy", _parse_greedy);
}
