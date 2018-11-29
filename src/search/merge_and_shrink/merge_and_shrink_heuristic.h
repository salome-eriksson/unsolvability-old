#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"
#include "../unsolvability/cudd_interface.h"

#include <memory>

namespace merge_and_shrink {
class MergeAndShrinkRepresentation;

class MergeAndShrinkHeuristic : public Heuristic {
    // The final merge-and-shrink representation, storing goal distances.
    std::unique_ptr<MergeAndShrinkRepresentation> mas_representation;

    // TODO: change vars as needed
    bool unsolvability_setup;
    std::vector<int> variable_order;
    CuddManager* cudd_manager;
    int setid;
    int k_set_dead;
    std::string bdd_filename;

    void setup_unsolvability_proof(UnsolvabilityManager &unsolvmanager);
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);

    virtual std::pair<int,int> prove_superset_dead(
            EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) override;
};
}

#endif
