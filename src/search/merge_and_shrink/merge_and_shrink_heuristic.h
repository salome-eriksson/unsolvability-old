#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"
#include "../unsolvability/cudd_interface.h"

#include <memory>

namespace Utils {
class Timer;
}


namespace MergeAndShrink {
class FactoredTransitionSystem;
class LabelReduction;
class MergeStrategy;
class ShrinkStrategy;
class TransitionSystem;

class MergeAndShrinkHeuristic : public Heuristic {
    // TODO: when the option parser supports it, the following should become
    // unique pointers.
    std::shared_ptr<MergeStrategy> merge_strategy;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    std::shared_ptr<LabelReduction> label_reduction;
    long starting_peak_memory;
    std::vector<int> variable_order;

    CuddManager* cudd_manager;
    CuddBDD* certificate;

    std::unique_ptr<FactoredTransitionSystem> fts;
    void build_transition_system(const Utils::Timer &timer);

    void report_peak_memory_delta(bool final = false) const;
    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const Options &opts);
    ~MergeAndShrinkHeuristic() = default;
    virtual void build_unsolvability_certificate(const GlobalState &);
    virtual int get_number_of_unsolvability_certificates();
    virtual void write_subcertificates(std::ofstream &cert_file);
};
}

#endif
