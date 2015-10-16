#include "task.h"
#include <algorithm>
#include <cassert>
#include <iostream>

Task::Task(std::string ) {
    fact_names.push_back("Atom_airborne(airplane_daewh_seg_rwe_0_50)");
    fact_names.push_back("Atom_not_blocked(seg_twe4_0_50_airplane_daewh)");
    fact_names.push_back("Atom_not_occupied(seg_pp_0_60)");
    fact_names.push_back("Atom_test2");
    fact_names.push_back("Atom_test4");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_tww3_0_50)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_tww4_0_50)");
    fact_names.push_back("Atom_is-moving(airplane_daewh)");
    fact_names.push_back("Atom_is-parked(airplane_daewh_seg_pp_0_60)");
    fact_names.push_back("Atom_not_blocked(seg_rw_0_400_airplane_daewh)");
    fact_names.push_back("Atom_test1");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_tww2_0_50)");
    fact_names.push_back("Atom_is-pushing(airplane_daewh)");
    fact_names.push_back("Atom_not_blocked(seg_ppdoor_0_40_airplane_daewh)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_ppdoor_0_40)");
    fact_names.push_back("Atom_not_occupied(seg_tww1_0_200)");
    fact_names.push_back("Atom_facing(airplane_daewh_north)");
    fact_names.push_back("Atom_not_occupied(seg_tww3_0_50)");
    fact_names.push_back("Atom_facing(airplane_daewh_south)");
    fact_names.push_back("Atom_not_blocked(seg_tww2_0_50_airplane_daewh)");
    fact_names.push_back("Atom_not_blocked(seg_pp_0_60_airplane_daewh)");
    fact_names.push_back("Atom_not_occupied(seg_tww4_0_50)");
    fact_names.push_back("Atom_not_blocked(seg_rwe_0_50_airplane_daewh)");
    fact_names.push_back("Atom_not_blocked(seg_tww4_0_50_airplane_daewh)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_rw_0_400)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_twe1_0_200)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_twe2_0_50)");
    fact_names.push_back("Atom_not_blocked(seg_rww_0_50_airplane_daewh)");
    fact_names.push_back("Atom_not_blocked(seg_tww3_0_50_airplane_daewh)");
    fact_names.push_back("Atom_not_blocked(seg_twe1_0_200_airplane_daewh)");
    fact_names.push_back("Atom_not_occupied(seg_rww_0_50)");
    fact_names.push_back("Atom_airborne(airplane_daewh_seg_rww_0_50)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_twe4_0_50)");
    fact_names.push_back("Atom_not_blocked(seg_twe3_0_50_airplane_daewh)");
    fact_names.push_back("Atom_not_blocked(seg_twe2_0_50_airplane_daewh)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_tww1_0_200)");
    fact_names.push_back("Atom_not_occupied(seg_rwe_0_50)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_rww_0_50)");
    fact_names.push_back("Atom_not_occupied(seg_ppdoor_0_40)");
    fact_names.push_back("Atom_not_blocked(seg_tww1_0_200_airplane_daewh)");
    fact_names.push_back("Atom_not_occupied(seg_twe1_0_200)");
    fact_names.push_back("Atom_not_occupied(seg_rw_0_400)");
    fact_names.push_back("Atom_not_occupied(seg_twe4_0_50)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_pp_0_60)");
    fact_names.push_back("Atom_not_occupied(seg_twe3_0_50)");
    fact_names.push_back("Atom_not_occupied(seg_tww2_0_50)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_rwe_0_50)");
    fact_names.push_back("Atom_at-segment(airplane_daewh_seg_twe3_0_50)");
    fact_names.push_back("Atom_test3");
    fact_names.push_back("Atom_not_occupied(seg_twe2_0_50)");
    fact_names.push_back("Atom_test5");
}

const std::string& Task::get_fact(int n) {
    return fact_names[n];
}

int Task::find_fact(std::string &fact_name) {
    int pos = find(fact_names.begin(), fact_names.end(), fact_name) - fact_names.begin();
    assert(pos < fact_names.size());
    return pos;
}

const Action& Task::get_action(int n) {
  return actions[n];
}

int Task::get_number_of_actions() {
  return actions.size();
}

int Task::get_number_of_facts() {
  return fact_names.size();
}

const State& Task::get_initial_state() {
  return initial_state;
}

const std::vector<int>& Task::get_goal() {
  return goal;
}
