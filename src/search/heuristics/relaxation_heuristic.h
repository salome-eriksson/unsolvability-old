#ifndef HEURISTICS_RELAXATION_HEURISTIC_H
#define HEURISTICS_RELAXATION_HEURISTIC_H

#include "array_pool.h"

#include "../heuristic.h"
#include "../unsolvability/cudd_interface.h"
#include "../evaluation_context.h"

#include "../utils/collections.h"

#include <cassert>
#include <vector>

class FactProxy;
class GlobalState;
class OperatorProxy;

namespace relaxation_heuristic {
struct Proposition;
struct UnaryOperator;

using PropID = int;
using OpID = int;

const OpID NO_OP = -1;

struct Proposition {
    Proposition();
    int cost; // used for h^max cost or h^add cost
    // TODO: Make sure in constructor that reached_by does not overflow.
    OpID reached_by : 30;
    /* The following two variables are conceptually bools, but Visual C++ does
       not support packing ints and bools together in a bitfield. */
    unsigned int is_goal : 1;
    unsigned int marked : 1; // used for preferred operators of h^add and h^FF
    int num_precondition_occurences;
    array_pool::ArrayPoolIndex precondition_of;
};

static_assert(sizeof(Proposition) == 16, "Proposition has wrong size");

struct UnaryOperator {
    UnaryOperator(int num_preconditions,
                  array_pool::ArrayPoolIndex preconditions,
                  PropID effect,
                  int operator_no, int base_cost);
    int cost; // Used for h^max cost or h^add cost;
              // includes operator cost (base_cost)
    int unsatisfied_preconditions;
    PropID effect;
    int base_cost;
    int num_preconditions;
    array_pool::ArrayPoolIndex preconditions;
    int operator_no; // -1 for axioms; index into the task's operators otherwise
};

static_assert(sizeof(UnaryOperator) == 28, "UnaryOperator has wrong size");

class RelaxationHeuristic : public Heuristic {
    void build_unary_operators(const OperatorProxy &op);
    void simplify();

    // proposition_offsets[var_no]: first PropID related to variable var_no
    std::vector<PropID> proposition_offsets;
protected:
    std::vector<UnaryOperator> unary_operators;
    std::vector<Proposition> propositions;
    std::vector<PropID> goal_propositions;

    bool unsolv_subsumption_check;
    CuddManager *cudd_manager;
    std::vector<CuddBDD> bdds;
    std::vector<int> bdd_to_stateid;
    std::unordered_map<int,int> state_to_bddindex;
    /*
      the first int is a setid which corresponds to the BDD in the bdds vector
      the second int is a knowledgeid which corresponds to the knowledge that setid is dead
     */
    std::vector<std::pair<int,int>> set_and_knowledge_ids;
    std::string bdd_filename;
    bool unsolvability_setup;

    array_pool::ArrayPool preconditions_pool;
    array_pool::ArrayPool precondition_of_pool;

    array_pool::ArrayPoolSlice get_preconditions(OpID op_id) const {
        const UnaryOperator &op = unary_operators[op_id];
        return preconditions_pool.get_slice(op.preconditions, op.num_preconditions);
    }

    // HACK!
    std::vector<PropID> get_preconditions_vector(OpID op_id) const {
        auto view = get_preconditions(op_id);
        return std::vector<PropID>(view.begin(), view.end());
    }

    /*
      TODO: Some of these protected methods are only needed for the
      CEGAR hack in the additive heuristic and should eventually go
      away.
    */
    PropID get_prop_id(const Proposition &prop) const {
        PropID prop_id = &prop - propositions.data();
        assert(utils::in_bounds(prop_id, propositions));
        return prop_id;
    }

    OpID get_op_id(const UnaryOperator &op) const {
        OpID op_id = &op - unary_operators.data();
        assert(utils::in_bounds(op_id, unary_operators));
        return op_id;
    }

    PropID get_prop_id(int var, int value) const;
    PropID get_prop_id(const FactProxy &fact) const;

    Proposition *get_proposition(PropID prop_id) {
        return &propositions[prop_id];
    }
    UnaryOperator *get_operator(OpID op_id) {
        return &unary_operators[op_id];
    }

    const Proposition *get_proposition(int var, int value) const;
    Proposition *get_proposition(int var, int value);
    Proposition *get_proposition(const FactProxy &fact);

    /*
      bool bdd_already_seen: whether the bdd was built before the function call already
      int bddindex: the index of the requested bdd in bdds vector
     */
    std::pair<bool,int> get_bdd_for_state(const GlobalState &state);
public:
    explicit RelaxationHeuristic(const options::Options &options);

    virtual bool dead_ends_are_reliable() const override;

    // functions related to unsolvability certificate generation
    virtual int create_subcertificate(EvaluationContext &eval_context) override;
    virtual void write_subcertificates(const std::string &filename) override;

    // functions related to unsolvability proof generation
    virtual void store_deadend_info(EvaluationContext &eval_context) override;
    virtual std::pair<int,int> get_set_and_deadknowledge_id(
            EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) override;
    virtual void finish_unsolvability_proof() override;
};
}

#endif
