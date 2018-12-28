#include "merge_and_shrink_heuristic.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "merge_and_shrink_algorithm.h"
#include "merge_and_shrink_representation.h"
#include "transition_system.h"
#include "types.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

#include "../utils/markup.h"
#include "../utils/system.h"

#include <cassert>
#include <iostream>
#include <utility>
#include <unordered_map>

using namespace std;
using utils::ExitCode;

namespace merge_and_shrink {
MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const options::Options &opts)
    : Heuristic(opts), unsolvability_setup(false) {
    Verbosity verbosity = static_cast<Verbosity>(opts.get_enum("verbosity"));

    cout << "Initializing merge-and-shrink heuristic..." << endl;
    MergeAndShrinkAlgorithm algorithm(opts);
    FactoredTransitionSystem fts = algorithm.build_factored_transition_system(task_proxy);

    /*
      TODO: This constructor has quite a bit of fiddling with aspects of
      transition systems and the merge-and-shrink representation (checking
      whether distances have been computed; computing them) that we would
      like to have at a lower level. See also the TODO in
      factored_transition_system.h on improving the interface of that class
      (and also related classes like TransitionSystem etc).
    */

    int ts_index = -1;
    if (fts.get_num_active_entries() == 1) {
        /*
          We regularly finished the merge-and-shrink construction, i.e., we
          merged all transition systems and are left with one solvable
          transition system. This assumes that merges are always appended at
          the end.
        */
        int last_factor_index = fts.get_size() - 1;
        for (int index = 0; index < last_factor_index; ++index) {
            assert(!fts.is_active(index));
        }
        ts_index = last_factor_index;
    } else {
        // The computation stopped early because there is an unsolvable factor.
        for (int index = 0; index < fts.get_size(); ++index) {
            if (fts.is_active(index) && !fts.is_factor_solvable(index)) {
                ts_index = index;
                break;
            }
        }
        assert(ts_index != -1);
    }

    auto final_entry = fts.extract_factor(ts_index);
    cout << "Final transition system size: "
         << fts.get_transition_system(ts_index).get_size() << endl;

    mas_representation = move(final_entry.first);
    unique_ptr<Distances> final_distances = move(final_entry.second);
    if (!final_distances->are_goal_distances_computed()) {
        const bool compute_init = false;
        const bool compute_goal = true;
        final_distances->compute_distances(
            compute_init, compute_goal, verbosity);
    }
    assert(final_distances->are_goal_distances_computed());
    mas_representation->set_distances(*final_distances);
    cout << "Done initializing merge-and-shrink heuristic." << endl << endl;


    int amount_vars = task_proxy.get_variables().size();
    variable_order.reserve(amount_vars);
    mas_representation->fill_varorder(variable_order);

    // fill variable order to contain all variables
    // TODO: can we do this nicer?
    for(int i = 0; i < amount_vars; ++i) {
        if(find(variable_order.begin(), variable_order.end(), i) == variable_order.end()) {
            variable_order.push_back(i);
        }
    }

    cudd_manager = new CuddManager(task, variable_order);
}

int MergeAndShrinkHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    int cost = mas_representation->get_value(state);
    if (cost == PRUNED_STATE || cost == INF) {
        // If state is unreachable or irrelevant, we encountered a dead end.
        return DEAD_END;
    }
    return cost;
}


void MergeAndShrinkHeuristic::setup_unsolvability_proof() {
    std::unordered_map<int, CuddBDD> bdd_map;
    bdd_map.insert({0, CuddBDD(cudd_manager, false)});
    bdd_map.insert({-1, CuddBDD(cudd_manager, true)});
    bdd = mas_representation->get_deadend_bdd(cudd_manager, bdd_map, true);
    unsolvability_setup = true;
}

int MergeAndShrinkHeuristic::create_subcertificate(EvaluationContext &eval_context) {
    if(!unsolvability_setup) {
        setup_unsolvability_proof();
        bdd_to_stateid = eval_context.get_state().get_id().get_value();
    }
    return bdd_to_stateid;
}

void MergeAndShrinkHeuristic::write_subcertificates(const string &filename) {
    if(bdd) {
        std::vector<CuddBDD> bddvec(1,*bdd);
        std::vector<int> stateidvec(1,bdd_to_stateid);
        cudd_manager->dumpBDDs_certificate(bddvec, stateidvec, filename);
    } else {
        std::ofstream cert_stream;
        cert_stream.open(filename);
        cert_stream.close();
    }
}

std::vector<int> MergeAndShrinkHeuristic::get_varorder() {
    return variable_order;
}

std::pair<int,int> MergeAndShrinkHeuristic::prove_superset_dead(
        EvaluationContext &, UnsolvabilityManager &unsolvmanager) {
    if(!unsolvability_setup) {
        setup_unsolvability_proof();
        std::vector<CuddBDD>bdds(1,*bdd);
        delete bdd;

        std::stringstream ss;
        ss << unsolvmanager.get_directory() << this << ".bdd";
        bdd_filename = ss.str();
        cudd_manager->dumpBDDs(bdds, bdd_filename);

        setid = unsolvmanager.get_new_setid();

        std::ofstream &certstream = unsolvmanager.get_stream();

        certstream << "e " << setid << " b " << bdd_filename << " 0 ;\n";
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

        k_set_dead = unsolvmanager.get_new_knowledgeid();
        certstream << "k " << k_set_dead << " d " << setid << " d6 " << k_prog << " "
                   << unsolvmanager.get_k_empty_dead() << " " << k_set_and_goal_dead << "\n";
    }

    return std::make_pair(setid, k_set_dead);
}


static shared_ptr<Heuristic> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Merge-and-shrink heuristic",
        "This heuristic implements the algorithm described in the following "
        "paper:" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "Generalized Label Reduction for Merge-and-Shrink Heuristics",
            "https://ai.dmi.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
            "Proceedings of the 28th AAAI Conference on Artificial"
            " Intelligence (AAAI 2014)",
            "2358-2366",
            "AAAI Press 2014") + "\n" +
        "For a more exhaustive description of merge-and-shrink, see the journal "
        "paper" + utils::format_paper_reference(
            {"Malte Helmert", "Patrik Haslum", "Joerg Hoffmann", "Raz Nissim"},
            "Merge-and-Shrink Abstraction: A Method for Generating Lower Bounds"
            " in Factored State Spaces",
            "https://ai.dmi.unibas.ch/papers/helmert-et-al-jacm2014.pdf",
            "Journal of the ACM 61 (3)",
            "16:1-63",
            "2014") + "\n" +
        "Please note that the journal paper describes the \"old\" theory of "
        "label reduction, which has been superseded by the above conference "
        "paper and is no longer implemented in Fast Downward.\n\n"
        "The following paper describes how to improve the DFP merge strategy "
        "with tie-breaking, and presents two new merge strategies (dyn-MIASM "
        "and SCC-DFP):" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
            "https://ai.dmi.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
            "Proceedings of the 26th International Conference on Automated "
            "Planning and Scheduling (ICAPS 2016)",
            "294-298",
            "AAAI Press 2016"));
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported (but see note)");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes (but see note)");
    parser.document_property("consistent", "yes (but see note)");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.document_note(
        "Note",
        "Conditional effects are supported directly. Note, however, that "
        "for tasks that are not factored (in the sense of the JACM 2014 "
        "merge-and-shrink paper), the atomic transition systems on which "
        "merge-and-shrink heuristics are based are nondeterministic, "
        "which can lead to poor heuristics even when only perfect shrinking "
        "is performed.");
    parser.document_note(
        "Note",
        "When pruning unreachable states, admissibility and consistency is "
        "only guaranteed for reachable states and transitions between "
        "reachable states. While this does not impact regular A* search which "
        "will never encounter any unreachable state, it impacts techniques "
        "like symmetry-based pruning: a reachable state which is mapped to an "
        "unreachable symmetric state (which hence is pruned) would falsely be "
        "considered a dead-end and also be pruned, thus violating optimality "
        "of the search.");
    parser.document_note(
        "Note",
        "A currently recommended good configuration uses bisimulation "
        "based shrinking, the merge strategy SCC-DFP, and the appropriate "
        "label reduction setting (max_states has been altered to be between "
        "10000 and 200000 in the literature):\n"
        "{{{\nmerge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),"
        "merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector="
        "score_based_filtering(scoring_functions=[goal_relevance,dfp,"
        "total_order])),label_reduction=exact(before_shrinking=true,"
        "before_merging=false),max_states=50000,threshold_before_merge=1)\n}}}\n"
        "Note that for versions of Fast Downward prior to 2016-08-19, the "
        "syntax differs. See the recommendation in the file "
        "merge_and_shrink_heuristic.cc for an example configuration.");

    Heuristic::add_options_to_parser(parser);
    add_merge_and_shrink_algorithm_options_to_parser(parser);
    options::Options opts = parser.parse();
    if (parser.help_mode()) {
        return nullptr;
    }

    handle_shrink_limit_options_defaults(opts);

    if (parser.dry_run()) {
        return nullptr;
    } else {
        return make_shared<MergeAndShrinkHeuristic>(opts);
    }
}

static options::Plugin<Evaluator> _plugin("merge_and_shrink", _parse);
}
