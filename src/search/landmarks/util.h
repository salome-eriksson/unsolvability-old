#ifndef LANDMARKS_UTIL_H
#define LANDMARKS_UTIL_H

#include "../utils/hash.h"

#include <vector>

class OperatorProxy;
class TaskProxy;

namespace landmarks {
class LandmarkNode;

utils::HashMap<int, int> _intersect(
    const utils::HashMap<int, int> &a,
    const utils::HashMap<int, int> &b);

bool _possibly_reaches_lm(const OperatorProxy &op,
                          const std::vector<std::vector<int>> &lvl_var,
                          const LandmarkNode *lmp);

OperatorProxy get_operator_or_axiom(const TaskProxy &task_proxy, int op_or_axiom_id);
int get_operator_or_axiom_id(const OperatorProxy &op);
}

#endif
