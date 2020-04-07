#include "actionset.h"

ActionSet::ActionSet()
{

}

ActionSetBasic::ActionSetBasic(std::unordered_set<int> &action_indices)
    : action_indices(action_indices) {

}
bool ActionSetBasic::contains(int ai) {
    return action_indices.find(ai) != action_indices.end();
}
bool ActionSetBasic::is_subset(ActionSet *other) {
    int ret = true;
    for(int elem : action_indices) {
        ret = other->contains(elem);
    }
}
void ActionSetBasic::get_actions(std::unordered_set<int> &set) {
    set.insert(action_indices.begin(), action_indices.end());
}
bool ActionSetBasic::is_constantall() {
    return false;
}

ActionSetUnion::ActionSetUnion(ActionSet *left, ActionSet *right)
    : left(left), right(right) {

}
bool ActionSetUnion::contains(int ai) {
    return (left->contains(ai) || right->contains(ai));
}
bool ActionSetUnion::is_subset(ActionSet *other) {
    return (left->is_subset(other) && right->is_subset(other));
}
void ActionSetUnion::get_actions(std::unordered_set<int> &set) {
    left->get_actions(set);
    right->get_actions(set);
}
bool ActionSetUnion::is_constantall() {
    return false;
}
int ActionSetUnion::get_left_id() {
    //TODO
    return -1;
}
int ActionSetUnion::get_right_id() {
    //TODO
    return -1;
}

ActionSetConstantAll::ActionSetConstantAll(Task &task)
    : action_amount(task.get_number_of_actions()) {

}
bool ActionSetConstantAll::contains(int ai) {
    return true;
}
bool ActionSetConstantAll::is_subset(ActionSet *other) {
    int ret = true;
    for(int i = 0; i < action_amount; ++i) {
        ret = other->contains(i);
    }
}
void ActionSetConstantAll::get_actions(std::unordered_set<int> &set) {
    for(int i = 0; i < action_amount; ++i) {
        set.insert(i);
    }
}
bool ActionSetConstantAll::is_constantall() {
    return true;
}
