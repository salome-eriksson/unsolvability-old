#include "eager_search.h"

#include "search_common.h"

#include "../evaluation_context.h"
#include "../globals.h"
#include "../heuristic.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../successor_generator.h"
#include "../utils/timer.h"

#include "../open_lists/open_list_factory.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <set>
#include <fstream>
#include <stdlib.h>

using namespace std;
static inline int get_op_index(const GlobalOperator *op) {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < static_cast<int>(g_operators.size()));
    return op_index;

}

namespace EagerSearch {
EagerSearch::EagerSearch(const Options &opts)
    : SearchEngine(opts),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      use_multi_path_dependence(opts.get<bool>("mpd")),
      open_list(opts.get<shared_ptr<OpenListFactory>>("open")->
                create_state_open_list()),
      f_evaluator(opts.get<ScalarEvaluator *>("f_eval", nullptr)),
      preferred_operator_heuristics(opts.get_list<Heuristic *>("preferred")) {
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

    const GlobalState &initial_state = g_initial_state();
    // Note: we consider the initial state as reached by a preferred
    // operator.
    EvaluationContext eval_context(initial_state, 0, true, &statistics);

    statistics.inc_evaluated_states();

    // initial_dead is introduced because we need to make sure that
    // the heuristics are initialized before getting the manager
    // (because the heuristics set the variable order)
    bool initial_dead = open_list->is_dead_end(eval_context);
    manager = CuddManager::get_instance();
    // for the maia grid, use a directory of another file system
    directory = "";
    srand(time(NULL));
    rand_number = rand();
    std::ofstream cert_file;
    cert_file.open("certificate.txt");
    hint_file.open(directory + "hints-" + std::to_string(rand_number) + ".txt");
    amount_vars = manager->get_amount_vars();
    fact_to_var = manager->get_fact_to_var();
    // there is currently no safeguard that these are the actual names used
    cert_file << "disjunctive_certificate\n";
    cert_file << "State BDDs:" << directory << "states-"
              << std::to_string(rand_number) << ".bdd\n";
    cert_file << "Heuristic Certificates BDDs:" << directory << "h_cert-"
              << std::to_string(rand_number) << ".bdd\n";
    cert_file << "hints:" << directory << "hints-"
              << std::to_string(rand_number) << ".txt\n";
    cert_file.close();

    if (initial_dead) {
        heuristics[0]->build_unsolvability_certificate(initial_state);
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
}

SearchStatus EagerSearch::step() {
    pair<SearchNode, bool> n = fetch_next_node();
    if (!n.second) {
        double writing_start = Utils::g_timer();
        std::string hcerts_filename = directory + "h_cert-" + std::to_string(rand_number) + ".bdd";
        heuristics[0]->write_subcertificates(hcerts_filename);
        write_statebdds();
        manager->writeTaskFile();
        hint_file << "end hints";
        hint_file.close();
        double writing_end = Utils::g_timer();
        std::cout << "Time for writing unsolvability certificate: " << writing_end - writing_start << std::endl;
        return FAILED;
    }
    SearchNode node = n.first;

    GlobalState s = node.get_state();
    if (check_goal_and_set_plan(s))
        return SOLVED;

    vector<const GlobalOperator *> applicable_ops;
    set<const GlobalOperator *> preferred_ops;

    g_successor_generator->generate_applicable_ops(s, applicable_ops);
    // This evaluates the expanded state (again) to get preferred ops
    EvaluationContext eval_context(s, node.get_g(), false, &statistics, true);
    for (Heuristic *heur : preferred_operator_heuristics) {
        /* In an alternation search with unreliable heuristics, it is
           possible that this heuristic considers the state a dead
           end. We only want to ask for preferred operators for
           finite-value heuristics. */
        if (!eval_context.is_heuristic_infinite(heur)) {
            vector<const GlobalOperator *> preferred =
                eval_context.get_preferred_operators(heur);
            preferred_ops.insert(preferred.begin(), preferred.end());
        }
    }

    // TODO: it is very distributed atm where in the code the hints are actually written
    hint_file << s.get_id().hash() << " " << applicable_ops.size();
    for (const GlobalOperator *op : applicable_ops) {
        if ((node.get_real_g() + op->get_cost()) >= bound) {
            hint_file << " " << get_op_index(op) << " -1";
            continue;
        }

        GlobalState succ_state = g_state_registry->get_successor_state(s, *op);
        statistics.inc_generated();
        bool is_preferred = (preferred_ops.find(op) != preferred_ops.end());

        SearchNode succ_node = search_space.get_node(succ_state);

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end()) {
            int hint = heuristics[0]->build_unsolvability_certificate(succ_state);
            hint_file << " " << get_op_index(op) << " " << hint;
            continue;
        }

        // update new path
        if (use_multi_path_dependence || succ_node.is_new()) {
            /*
              Note: we must call reach_state for each heuristic, so
              don't break out of the for loop early.
            */
            for (Heuristic *heuristic : heuristics) {
                heuristic->reach_state(s, *op, succ_state);
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
                int hint = heuristics[0]->build_unsolvability_certificate(succ_state);
                hint_file << " " << get_op_index(op) << " " << hint;
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
        hint_file << " " << get_op_index(op) << " " << succ_state.get_id().hash();
    }
    hint_file << "\n";

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
            SearchNode dummy_node = search_space.get_node(g_initial_state());
            return make_pair(dummy_node, false);
        }
        vector<int> last_key_removed;
        StateID id = open_list->remove_min(
            use_multi_path_dependence ? &last_key_removed : nullptr);
        // TODO is there a way we can avoid creating the state here and then
        //      recreate it outside of this function with node.get_state()?
        //      One way would be to store GlobalState objects inside SearchNodes
        //      instead of StateIDs
        GlobalState s = g_state_registry->lookup_state(id);
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

void EagerSearch::write_statebdds() {
    std::string statebdd_file = directory + "states-" + std::to_string(rand_number) + ".bdd";
    std::ofstream stream;
    stream.open(statebdd_file);

    // loop over all states and insert closed states in statebdd_file
    for(const StateID id : *g_state_registry) {
        const GlobalState &state = g_state_registry->lookup_state(id);
        if(search_space.get_node(state).is_closed()) {
            dump_state_bdd(state, stream);
        }
    }

    stream.close();
}

void EagerSearch::dump_state_bdd(const GlobalState &s, std::ofstream &statebdd_file) {
    // first dump index
    statebdd_file << s.get_id().hash() << "\n";

    // header
    statebdd_file << ".ver DDDMP-2.0\n";
    statebdd_file << ".mode A\n";
    statebdd_file << ".varinfo 0\n";
    statebdd_file << ".nnodes " << amount_vars+1 << "\n";
    statebdd_file << ".nvars " << amount_vars << "\n";
    statebdd_file << ".nsuppvars " << amount_vars << "\n";
    statebdd_file << ".ids";
    for(int i = 0; i < amount_vars; ++i) {
        statebdd_file << " " << i;
    }
    statebdd_file << "\n";
    statebdd_file << ".permids";
    for(int i = 0; i < amount_vars; ++i) {
        statebdd_file << " " << i;
    }
    statebdd_file << "\n";
    statebdd_file << ".nroots 1\n";
    statebdd_file << ".rootids -" << amount_vars+1 << "\n";
    statebdd_file << ".nodes\n";

    // nodes
    // TODO: this is a huge mess because only "false" arcs can be minus
    // We start with a negative root, and if the last var is false, we can
    // put a minus in the last arc and reach true in this way.
    // if the last var is true, we assume that the second last is false (because FDR)
    // and thus put a minus on the second last arc which means the last node is "positive"
    std::vector<bool> state_vars(amount_vars, false);
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        state_vars[(*fact_to_var)[i][s[i]]] = true;
    }
    assert(!state_vars[amount_vars-1] || !state_vars[amount_vars-2]);
    bool last_true = state_vars[amount_vars-1];
    int var = amount_vars-1;

    statebdd_file << "1 T 1 0 0\n";
    statebdd_file << "2 " << var << " " << var << " 1 -1\n";
    var--;
    statebdd_file << "3 " << var << " " << var << " ";
    if(last_true) {
        statebdd_file << "1 -2\n";
    } else if(state_vars[var]) {
        statebdd_file << "2 1\n";
    } else {
        statebdd_file << "1 2\n";
    }
    var--;

    for(int i = 4; i <= amount_vars+1; ++i) {
        statebdd_file << i << " " << var << " " << var << " ";
        if(state_vars[var]) {
            statebdd_file << i-1 << " 1\n";
        } else {
            statebdd_file << "1 " << i-1 << "\n";
        }
        var--;
    }
    statebdd_file << ".end\n";
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
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    EagerSearch *engine = nullptr;
    if (!parser.dry_run()) {
        auto temp = SearchCommon::create_astar_open_list_factory_and_f_eval(opts);
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
    SearchEngine::add_options_to_parser(parser);

    Options opts = parser.parse();
    opts.verify_list_non_empty<ScalarEvaluator *>("evals");

    EagerSearch *engine = nullptr;
    if (!parser.dry_run()) {
        opts.set("open", SearchCommon::create_greedy_open_list_factory(opts));
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
