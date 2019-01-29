#ifndef HEURISTICS_RELAXATION_HEURISTIC_H
#define HEURISTICS_RELAXATION_HEURISTIC_H

#include "../heuristic.h"
#include "../unsolvability/cudd_interface.h"
#include "../evaluation_context.h"

#include <vector>

class FactProxy;
class GlobalState;
class OperatorProxy;

namespace relaxation_heuristic {
struct Proposition;
struct UnaryOperator;

struct UnaryOperator {
    int operator_no; // -1 for axioms; index into the task's operators otherwise
    std::vector<Proposition *> precondition;
    Proposition *effect;
    int base_cost;

    int unsatisfied_preconditions;
    int cost; // Used for h^max cost or h^add cost;
              // includes operator cost (base_cost)
    UnaryOperator(const std::vector<Proposition *> &pre, Proposition *eff,
                  int operator_no_, int base)
        : operator_no(operator_no_), precondition(pre), effect(eff),
          base_cost(base) {}
};

struct Proposition {
    bool is_goal;
    int id;
    std::vector<UnaryOperator *> precondition_of;

    int cost; // Used for h^max cost or h^add cost
    UnaryOperator *reached_by;
    bool marked; // used when computing preferred operators for h^add and h^FF

    Proposition(int id_) {
        id = id_;
        is_goal = false;
        cost = -1;
        reached_by = 0;
        marked = false;
    }
};

class RelaxationHeuristic : public Heuristic {
    void build_unary_operators(const OperatorProxy &op, int op_no);
    void simplify();
protected:
    std::vector<UnaryOperator> unary_operators;
    std::vector<std::vector<Proposition>> propositions;
    std::vector<Proposition *> goal_propositions;

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

    Proposition *get_proposition(const FactProxy &fact);
    virtual int compute_heuristic(const GlobalState &state) = 0;

    /*
      bool bdd_already_seen: whether the bdd was built before the function call already
      int bddindex: the index of the requested bdd in bdds vector
     */
    std::pair<bool,int> get_bdd_for_state(const GlobalState &state);
public:
    RelaxationHeuristic(const options::Options &opts);
    virtual ~RelaxationHeuristic();
    virtual bool dead_ends_are_reliable() const;

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
