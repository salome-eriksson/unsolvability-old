#include "merge_and_shrink_representation.h"

#include "distances.h"
#include "types.h"

#include "../task_proxy.h"

#include <algorithm>
#include <iostream>
#include <numeric>

using namespace std;

namespace merge_and_shrink {
MergeAndShrinkRepresentation::MergeAndShrinkRepresentation(int domain_size)
    : domain_size(domain_size) {
}

MergeAndShrinkRepresentation::~MergeAndShrinkRepresentation() {
}

int MergeAndShrinkRepresentation::get_domain_size() const {
    return domain_size;
}


MergeAndShrinkRepresentationLeaf::MergeAndShrinkRepresentationLeaf(
    int var_id, int domain_size)
    : MergeAndShrinkRepresentation(domain_size),
      var_id(var_id),
      lookup_table(domain_size) {
    iota(lookup_table.begin(), lookup_table.end(), 0);
}

void MergeAndShrinkRepresentationLeaf::set_distances(
    const Distances &distances) {
    for (int &entry : lookup_table) {
        if (entry != PRUNED_STATE) {
            entry = distances.get_goal_distance(entry);
        }
    }
}

void MergeAndShrinkRepresentationLeaf::apply_abstraction_to_lookup_table(
    const vector<int> &abstraction_mapping) {
    int new_domain_size = 0;
    for (int &entry : lookup_table) {
        if (entry != PRUNED_STATE) {
            entry = abstraction_mapping[entry];
            new_domain_size = max(new_domain_size, entry + 1);
        }
    }
    domain_size = new_domain_size;
}

int MergeAndShrinkRepresentationLeaf::get_value(const State &state) const {
    int value = state[var_id].get_value();
    return lookup_table[value];
}

void MergeAndShrinkRepresentationLeaf::dump() const {
    for (const auto &value : lookup_table) {
        cout << value << ", ";
    }
    cout << endl;
}

void MergeAndShrinkRepresentationLeaf::get_unsolvability_certificate(CuddBDD* h_inf,
            std::vector<CuddBDD> &bdd_for_val, bool fill_bdd_for_val) {
    int val;
    for(size_t i = 0; i < lookup_table.size(); ++i) {
        val = lookup_table[i];
        CuddBDD fact_bdd(h_inf->get_manager(), var_id, i, false);
        for(size_t x = 0; x < lookup_table.size(); ++x) {
            if(x == i) {
                continue;
            }
            fact_bdd.land(var_id, x, true);
        }
        if(val == -1) {
            h_inf->lor(fact_bdd);
        } else if(fill_bdd_for_val) {
            bdd_for_val[val].lor(fact_bdd);
        }
    }
}

MergeAndShrinkRepresentationMerge::MergeAndShrinkRepresentationMerge(
    unique_ptr<MergeAndShrinkRepresentation> left_child_,
    unique_ptr<MergeAndShrinkRepresentation> right_child_)
    : MergeAndShrinkRepresentation(left_child_->get_domain_size() *
                                   right_child_->get_domain_size()),
      left_child(move(left_child_)),
      right_child(move(right_child_)),
      lookup_table(left_child->get_domain_size(),
                   vector<int>(right_child->get_domain_size())) {
    int counter = 0;
    for (vector<int> &row : lookup_table) {
        for (int &entry : row) {
            entry = counter;
            ++counter;
        }
    }
}

void MergeAndShrinkRepresentationMerge::set_distances(
    const Distances &distances) {
    for (vector<int> &row : lookup_table) {
        for (int &entry : row) {
            if (entry != PRUNED_STATE) {
                entry = distances.get_goal_distance(entry);
            }
        }
    }
}

void MergeAndShrinkRepresentationMerge::apply_abstraction_to_lookup_table(
    const vector<int> &abstraction_mapping) {
    int new_domain_size = 0;
    for (vector<int> &row : lookup_table) {
        for (int &entry : row) {
            if (entry != PRUNED_STATE) {
                entry = abstraction_mapping[entry];
                new_domain_size = max(new_domain_size, entry + 1);
            }
        }
    }
    domain_size = new_domain_size;
}

int MergeAndShrinkRepresentationMerge::get_value(
    const State &state) const {
    int state1 = left_child->get_value(state);
    int state2 = right_child->get_value(state);
    if (state1 == PRUNED_STATE ||
        state2 == PRUNED_STATE)
        return PRUNED_STATE;
    return lookup_table[state1][state2];
}

void MergeAndShrinkRepresentationMerge::dump() const {
    for (const auto &row : lookup_table) {
        for (const auto &value : row) {
            cout << value << ", ";
        }
        cout << endl;
    }
    cout << "dump left child:" << endl;
    left_child->dump();
    cout << "dump right child:" << endl;
    right_child->dump();
}

/*
 * h_inf: states with infinite estimate
 * bdd_for_val: is filled with bdds for each value (each bdd represents the
 * variable assignment so far that maps to the value in the lookup_table for the current
 * HeuristicRepresentation)
 * fill_bdd_for_val: if bdd_for_val should be filled (this will not be necessary for the "root"
 * table, since we are only interested in the states with infinite estimate
 */
void MergeAndShrinkRepresentationMerge::get_unsolvability_certificate(CuddBDD* h_inf,
         std::vector<CuddBDD> &bdd_for_val, bool fill_bdd_for_val) {
    CuddManager* manager = h_inf->get_manager();
    size_t rows = lookup_table.size();
    size_t columns = lookup_table[0].size();
    //get the bdds for the child nodes
    std::vector<CuddBDD> left_child_bdds(rows, CuddBDD(manager, false));
    std::vector<CuddBDD> right_child_bdds(columns, CuddBDD(manager, false));
    left_child->get_unsolvability_certificate(h_inf, left_child_bdds, true);
    right_child->get_unsolvability_certificate(h_inf, right_child_bdds, true);


    int val;
    CuddBDD tmp(manager);
    for(size_t i = 0; i < rows; ++i) {
        for(size_t j = 0; j < columns; ++j) {
            val = lookup_table[i][j];
            if(val == -1) {
                tmp = left_child_bdds[i];
                tmp.land(right_child_bdds[j]);
                h_inf->lor(tmp);
            } else if(fill_bdd_for_val) {
                tmp = left_child_bdds[i];
                tmp.land(right_child_bdds[j]);
                bdd_for_val[val].lor(tmp);
            }
        }
    }
}
}
