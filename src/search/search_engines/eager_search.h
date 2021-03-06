#ifndef SEARCH_ENGINES_EAGER_SEARCH_H
#define SEARCH_ENGINES_EAGER_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"

#include "../unsolvability/cudd_interface.h"

#include <memory>
#include <vector>

class Evaluator;
class PruningMethod;

namespace options {
class OptionParser;
class Options;
}

enum class UnsolvabilityVerificationType {
    NONE,
    CERTIFICATE,
    CERTIFICATE_FASTDUMP,
    CERTIFICATE_NOHINTS,
    PROOF,
    PROOF_DISCARD
};

namespace eager_search {
class EagerSearch : public SearchEngine {
    const bool reopen_closed_nodes;
    const UnsolvabilityVerificationType unsolv_type;

    std::unique_ptr<StateOpenList> open_list;
    std::shared_ptr<Evaluator> f_evaluator;

    std::vector<Evaluator *> path_dependent_evaluators;
    std::vector<std::shared_ptr<Evaluator>> preferred_operator_evaluators;
    std::shared_ptr<Evaluator> lazy_evaluator;

    std::shared_ptr<PruningMethod> pruning_method;

    void start_f_value_statistics(EvaluationContext &eval_context);
    void update_f_value_statistics(EvaluationContext &eval_context);
    void reward_progress();

    std::string unsolvability_directory;
    std::ofstream unsolvability_certificate_hints;

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit EagerSearch(const options::Options &opts);
    virtual ~EagerSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;

    void write_unsolvability_certificate();
    void write_unsolvability_proof();
    void write_unsolvability_task_file(const std::vector<int> &varorder);
};

extern void add_options_to_parser(options::OptionParser &parser);
}

#endif
