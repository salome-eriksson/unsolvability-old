#include "tiebreaking_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/memory.h"

#include <cassert>
#include <deque>
#include <map>
#include <utility>
#include <vector>

using namespace std;

namespace tiebreaking_open_list {
template<class Entry>
class TieBreakingOpenList : public OpenList<Entry> {
    using Bucket = deque<Entry>;

    map<const vector<int>, Bucket> buckets;
    int size;

    vector<shared_ptr<Evaluator>> evaluators;
    /*
      If allow_unsafe_pruning is true, we ignore (don't insert) states
      which the first evaluator considers a dead end, even if it is
      not a safe heuristic.
    */
    bool allow_unsafe_pruning;

    int dimension() const;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit TieBreakingOpenList(const Options &opts);
    virtual ~TieBreakingOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;

    virtual int create_subcertificate(EvaluationContext &eval_context) override;
    virtual void write_subcertificates(const std::string &filename) override;
    virtual std::vector<int> get_varorder() override;

    virtual std::pair<int,int> prove_superset_dead(
            EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) override;
    virtual void finish_unsolvability_proof() override;
};


template<class Entry>
TieBreakingOpenList<Entry>::TieBreakingOpenList(const Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      size(0), evaluators(opts.get_list<shared_ptr<Evaluator>>("evals")),
      allow_unsafe_pruning(opts.get<bool>("unsafe_pruning")) {
}

template<class Entry>
void TieBreakingOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    vector<int> key;
    key.reserve(evaluators.size());
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        key.push_back(eval_context.get_evaluator_value_or_infinity(evaluator.get()));

    buckets[key].push_back(entry);
    ++size;
}

template<class Entry>
Entry TieBreakingOpenList<Entry>::remove_min() {
    assert(size > 0);
    typename map<const vector<int>, Bucket>::iterator it;
    it = buckets.begin();
    assert(it != buckets.end());
    assert(!it->second.empty());
    --size;
    Entry result = it->second.front();
    it->second.pop_front();
    if (it->second.empty())
        buckets.erase(it);
    return result;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TieBreakingOpenList<Entry>::clear() {
    buckets.clear();
    size = 0;
}

template<class Entry>
int TieBreakingOpenList<Entry>::dimension() const {
    return evaluators.size();
}

template<class Entry>
void TieBreakingOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool TieBreakingOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // TODO: Properly document this behaviour.
    // If one safe heuristic detects a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // If the first heuristic detects a dead-end and we allow "unsafe
    // pruning", return true.
    if (allow_unsafe_pruning &&
        eval_context.is_evaluator_value_infinite(evaluators[0].get()))
        return true;
    // Otherwise, return true if all heuristics agree this is a dead-end.
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        if (!eval_context.is_evaluator_value_infinite(evaluator.get()))
            return false;
    return true;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        if (eval_context.is_evaluator_value_infinite(evaluator.get()) &&
            evaluator->dead_ends_are_reliable())
            return true;
    return false;
}

template<class Entry>
int TieBreakingOpenList<Entry>::create_subcertificate(EvaluationContext &eval_context) {
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        if (eval_context.is_evaluator_value_infinite(evaluator.get())) {
            return evaluator->create_subcertificate(eval_context);
        }
    }
    std::cerr << "Requested subcertificate for non-dead state." << std::endl;
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

template<class Entry>
void TieBreakingOpenList<Entry>::write_subcertificates(const std::string &filename) {
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        evaluator->write_subcertificates(filename);
    }
}

template<class Entry>
std::vector<int> TieBreakingOpenList<Entry>::get_varorder() {
    std::vector<int> varorder;
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        if(varorder.empty()) {
            varorder = evaluator->get_varorder();
        } else if(varorder != evaluator->get_varorder()) {
            std::cerr << "Different varorders in certificate!" << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
    return varorder;
}

template<class Entry>
std::pair<int,int> TieBreakingOpenList<Entry>::prove_superset_dead(
        EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager)  {
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        if (eval_context.is_evaluator_value_infinite(evaluator.get())) {
            return evaluator->prove_superset_dead(eval_context, unsolvmanager);
        }
    }
    std::cerr << "Requested proof of deadness for non-dead state." << std::endl;
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

template<class Entry>
void TieBreakingOpenList<Entry>::finish_unsolvability_proof() {
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        evaluator->finish_unsolvability_proof();
    }
}

TieBreakingOpenListFactory::TieBreakingOpenListFactory(const Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
TieBreakingOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<TieBreakingOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
TieBreakingOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<TieBreakingOpenList<EdgeOpenListEntry>>(options);
}

static shared_ptr<OpenListFactory> _parse(OptionParser &parser) {
    parser.document_synopsis("Tie-breaking open list", "");
    parser.add_list_option<shared_ptr<Evaluator>>("evals", "evaluators");
    parser.add_option<bool>(
        "pref_only",
        "insert only nodes generated by preferred operators", "false");
    parser.add_option<bool>(
        "unsafe_pruning",
        "allow unsafe pruning when the main evaluator regards a state a dead end",
        "true");
    Options opts = parser.parse();
    opts.verify_list_non_empty<shared_ptr<Evaluator>>("evals");
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<TieBreakingOpenListFactory>(opts);
}

static Plugin<OpenListFactory> _plugin("tiebreaking", _parse);
}
