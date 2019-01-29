#include "standard_scalar_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/memory.h"

#include <cassert>
#include <deque>
#include <map>

using namespace std;

namespace standard_scalar_open_list {
template<class Entry>
class StandardScalarOpenList : public OpenList<Entry> {
    typedef deque<Entry> Bucket;

    map<int, Bucket> buckets;
    int size;

    shared_ptr<Evaluator> evaluator;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit StandardScalarOpenList(const Options &opts);
    StandardScalarOpenList(const shared_ptr<Evaluator> &eval, bool preferred_only);
    virtual ~StandardScalarOpenList() override = default;

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

    virtual void store_deadend_info(EvaluationContext &eval_context) override;
    virtual std::pair<int,int> get_set_and_deadknowledge_id(
            EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) override;
    virtual void finish_unsolvability_proof() override;
};


template<class Entry>
StandardScalarOpenList<Entry>::StandardScalarOpenList(const Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      size(0),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
StandardScalarOpenList<Entry>::StandardScalarOpenList(
    const shared_ptr<Evaluator> &evaluator, bool preferred_only)
    : OpenList<Entry>(preferred_only),
      size(0),
      evaluator(evaluator) {
}

template<class Entry>
void StandardScalarOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    int key = eval_context.get_evaluator_value(evaluator.get());
    buckets[key].push_back(entry);
    ++size;
}

template<class Entry>
Entry StandardScalarOpenList<Entry>::remove_min() {
    assert(size > 0);
    auto it = buckets.begin();
    assert(it != buckets.end());
    Bucket &bucket = it->second;
    assert(!bucket.empty());
    Entry result = bucket.front();
    bucket.pop_front();
    if (bucket.empty())
        buckets.erase(it);
    --size;
    return result;
}

template<class Entry>
bool StandardScalarOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void StandardScalarOpenList<Entry>::clear() {
    buckets.clear();
    size = 0;
}

template<class Entry>
void StandardScalarOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool StandardScalarOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool StandardScalarOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
int StandardScalarOpenList<Entry>::create_subcertificate(EvaluationContext &eval_context) {
    if (eval_context.is_evaluator_value_infinite(evaluator.get())) {
        return evaluator->create_subcertificate(eval_context);
    }
    std::cerr << "Requested subcertificate for non-dead state." << std::endl;
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

template<class Entry>
void StandardScalarOpenList<Entry>::write_subcertificates(const std::string &filename) {
    evaluator->write_subcertificates(filename);
}

template<class Entry>
std::vector<int> StandardScalarOpenList<Entry>::get_varorder() {
    return evaluator->get_varorder();
}

template<class Entry>
void StandardScalarOpenList<Entry>::store_deadend_info(EvaluationContext &eval_context) {
    if (eval_context.is_evaluator_value_infinite(evaluator.get())) {
        evaluator->store_deadend_info(eval_context);
    }
}

template<class Entry>
std::pair<int,int> StandardScalarOpenList<Entry>::get_set_and_deadknowledge_id(
        EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) {
    if (eval_context.is_evaluator_value_infinite(evaluator.get())) {
        return evaluator->get_set_and_deadknowledge_id(eval_context, unsolvmanager);
    }
    std::cerr << "Requested proof of deadness for non-dead state." << std::endl;
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

template<class Entry>
void StandardScalarOpenList<Entry>::finish_unsolvability_proof() {
    evaluator->finish_unsolvability_proof();
}

StandardScalarOpenListFactory::StandardScalarOpenListFactory(
    const Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
StandardScalarOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<StandardScalarOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
StandardScalarOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<StandardScalarOpenList<EdgeOpenListEntry>>(options);
}

static shared_ptr<OpenListFactory> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Standard open list",
        "Standard open list that uses a single evaluator");
    parser.add_option<shared_ptr<Evaluator>>("eval", "evaluator");
    parser.add_option<bool>(
        "pref_only",
        "insert only nodes generated by preferred operators", "false");

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<StandardScalarOpenListFactory>(opts);
}

static Plugin<OpenListFactory> _plugin("single", _parse);
}
