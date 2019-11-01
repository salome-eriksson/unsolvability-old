#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"
#include "../evaluation_context.h"
#include "../unsolvability/cudd_interface.h"

#include <memory>

namespace utils {
enum class Verbosity;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeAndShrinkRepresentation;

class MergeAndShrinkHeuristic : public Heuristic {
    const utils::Verbosity verbosity;

    // The final merge-and-shrink representations, storing goal distances.
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> mas_representations;

    void extract_factor(FactoredTransitionSystem &fts, int index);
    bool extract_unsolvable_factor(FactoredTransitionSystem &fts);
    void extract_nontrivial_factors(FactoredTransitionSystem &fts);
    void extract_factors(FactoredTransitionSystem &fts);

    CuddManager* cudd_manager;
    std::vector<int> variable_order;
    CuddBDD *bdd;

    int bdd_to_stateid;

    int setid;
    int k_set_dead;
    std::string bdd_filename;

    virtual int create_subcertificate(EvaluationContext &eval_context) override;
    virtual void write_subcertificates(const std::string &filename) override;
    virtual std::vector<int> get_varorder() override;

    void get_bdd();
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);

    // currently not used
    //virtual void store_deadend_info(EvaluationContext &eval_context) override;
    virtual std::pair<int,int> get_set_and_deadknowledge_id(
            EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) override;
};
}

#endif
