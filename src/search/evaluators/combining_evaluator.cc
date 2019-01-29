#include "combining_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"

using namespace std;

namespace combining_evaluator {
CombiningEvaluator::CombiningEvaluator(
    const vector<shared_ptr<Evaluator>> &subevaluators_)
    : subevaluators(subevaluators_) {
    all_dead_ends_are_reliable = true;
    for (const shared_ptr<Evaluator> &subevaluator : subevaluators)
        if (!subevaluator->dead_ends_are_reliable())
            all_dead_ends_are_reliable = false;
}

CombiningEvaluator::~CombiningEvaluator() {
}

bool CombiningEvaluator::dead_ends_are_reliable() const {
    return all_dead_ends_are_reliable;
}

EvaluationResult CombiningEvaluator::compute_result(
    EvaluationContext &eval_context) {
    // This marks no preferred operators.
    EvaluationResult result;
    vector<int> values;
    values.reserve(subevaluators.size());

    // Collect component values. Return infinity if any is infinite.
    for (const shared_ptr<Evaluator> &subevaluator : subevaluators) {
        int value = eval_context.get_evaluator_value_or_infinity(subevaluator.get());
        if (value == EvaluationResult::INFTY) {
            result.set_evaluator_value(value);
            return result;
        } else {
            values.push_back(value);
        }
    }

    // If we arrived here, all subevaluator values are finite.
    result.set_evaluator_value(combine_values(values));
    return result;
}

void CombiningEvaluator::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (auto &subevaluator : subevaluators)
        subevaluator->get_path_dependent_evaluators(evals);
}

int CombiningEvaluator::create_subcertificate(EvaluationContext &eval_context) {
    for (const shared_ptr<Evaluator> &subevaluator : subevaluators) {
        if (eval_context.is_evaluator_value_infinite(subevaluator.get())) {
            return subevaluator->create_subcertificate(eval_context);
        }
    }
    std::cerr << "Requested subcertificate for non-dead state." << std::endl;
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

void CombiningEvaluator::store_deadend_info(EvaluationContext &eval_context) {
    for (const shared_ptr<Evaluator> &subevaluator : subevaluators) {
        if (eval_context.is_evaluator_value_infinite(subevaluator.get())) {
            subevaluator->store_deadend_info(eval_context);
            break;
        }
    }
}

std::pair<int,int> CombiningEvaluator::get_set_and_deadknowledge_id(
        EvaluationContext &eval_context, UnsolvabilityManager &unsolvmanager) {
    for (const shared_ptr<Evaluator> &subevaluator : subevaluators) {
        if (eval_context.is_evaluator_value_infinite(subevaluator.get())) {
            return subevaluator->get_set_and_deadknowledge_id(eval_context, unsolvmanager);
        }
    }
    std::cerr << "Requested proof of deadness for non-dead state." << std::endl;
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

}
