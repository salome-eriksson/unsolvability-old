#ifndef CEGAR_TRANSITION_UPDATER_H
#define CEGAR_TRANSITION_UPDATER_H

class AbstractTask;
class OperatorsProxy;

#include <memory>

namespace cegar {
class AbstractState;

/*
  Rewire transitions after each split.
*/
class TransitionUpdater {
    const std::shared_ptr<AbstractTask> task;

    int num_non_loops;
    int num_loops;

    OperatorsProxy get_operators() const;

    void add_transition(AbstractState *src, int op_id, AbstractState *target);
    void add_loop(AbstractState *state, int op_id);

    void remove_incoming_transition(
        AbstractState *src, int op_id, AbstractState *target);
    void remove_outgoing_transition(
        AbstractState *src, int op_id, AbstractState *target);

    void rewire_incoming_transitions(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
    void rewire_outgoing_transitions(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
    void rewire_loops(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);

public:
    explicit TransitionUpdater(const std::shared_ptr<AbstractTask> &task);

    void add_loops_to_trivial_abstract_state(AbstractState *state);

    // Update transition system after v has been split into v1 and v2.
    void rewire(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);

    int get_num_non_loops() const;
    int get_num_loops() const;
};
}

#endif
