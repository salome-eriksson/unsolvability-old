#include "knowledge.h"

Knowledge::~Knowledge() {}

SubsetKnowledge::SubsetKnowledge(int left_id, int right_id)
    : left_id(left_id), right_id(right_id) {

}

int SubsetKnowledge::get_left_id() {
    return left_id;
}

int SubsetKnowledge::get_right_id() {
    return right_id;
}


DeadKnowledge::DeadKnowledge(int set_id)
    : set_id(set_id) {

}

int DeadKnowledge::get_set_id() {
    return set_id;
}
