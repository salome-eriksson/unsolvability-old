#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"
#include "../evaluation_context.h"
#include "../unsolvability/cudd_interface.h"

#include <memory>

namespace merge_and_shrink {
class MergeAndShrinkRepresentation;

class MergeAndShrinkHeuristic : public Heuristic {
    // The final merge-and-shrink representation, storing goal distances.
    std::unique_ptr<MergeAndShrinkRepresentation> mas_representation;

    // TODO: change vars as needed
    bool unsolvability_setup;
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

    void setup_unsolvability_proof();
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);

    virtual std::pair<int,int> prove_superset_dead(
            EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) override;
};
}

#endif
