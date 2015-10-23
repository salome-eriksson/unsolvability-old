#include "task.h"
#include "global_funcs.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_map>

Task::Task(std::string taskfile) {

    std::ifstream in;
    in.open(taskfile.c_str());
    std::string line;
    std::vector<std::string> linevec;
    std::unordered_map<std::string, int> fact_map;

    //parse number of atoms
    std::getline(in, line);
    split(line, linevec, ':');
    assert(linevec.size() == 2);
    if(linevec[0].compare("begin_atoms") != 0) {
        print_parsing_error_and_exit(line, "begin_atoms:<numatoms>");
    }
    int factamount = stoi(linevec[1]);
    fact_names.resize(factamount);
    //parse atoms
    for(size_t i = 0; i < factamount; ++i) {
        std::getline(in, fact_names[i]);
        fact_map.insert({fact_names[i], i});
    }
    std::getline(in, line);
    if(line.compare("end_atoms") != 0) {
        print_parsing_error_and_exit(line, "end_atoms");
    }

    //parse initial state
    std::getline(in, line);
    if(line.compare("begin_init") != 0) {
        print_parsing_error_and_exit(line, "begin_init");
    }
    initial_state.resize(factamount, false);
    std::getline(in, line);
    while(line.compare("end_init") != 0) {
        auto index = fact_map.find(line);
        if(index == fact_map.end()) {
            print_parsing_error_and_exit(line, "<initial state atom>");
        }
        initial_state[index->second] = true;
        std::getline(in, line);
    }

    //parse goal
    std::getline(in, line);
    if(line.compare("begin_goal") != 0) {
        print_parsing_error_and_exit(line, "begin_goal");
    }
    std::getline(in, line);
    while(line.compare("end_goal") != 0) {
        auto index = fact_map.find(line);
        if(index == fact_map.end()) {
            print_parsing_error_and_exit(line, "<goal atom>");
        }
        goal.push_back(index->second);
        std::getline(in, line);
    }

    //parsing actions
    std::getline(in, line);
    split(line, linevec, ':');
    assert(linevec.size() == 2);
    if(linevec[0].compare("begin_actions") != 0) {
        print_parsing_error_and_exit(line, "begin_actions:<numactions>");
    }
    int actionamount = stoi(linevec[1]);
    actions.resize(actionamount);
    for(int i = 0; i < actionamount; ++i) {
        getline(in, line);
        if(line.compare("begin_action") != 0) {
            print_parsing_error_and_exit(line, "begin_action");
        }
        //parse name
        getline(in, line);
        actions[i].name = line;
        //parse cost
        getline(in, line);
        split(line, linevec, ':');
        if(linevec[0].compare("cost") != 0) {
            print_parsing_error_and_exit(line, "cost:<actioncost>");
        }
        actions[i].cost = stoi(linevec[1]);

        actions[i].change.resize(factamount, 0);
        getline(in, line);
        while(line.compare("end_action")!= 0) {
            split(line, linevec, ':');
            assert(linevec.size() == 2);
            int fact = fact_map.at(linevec[1]);
            if(linevec[0].compare("PRE") == 0) {
                actions[i].pre.push_back(fact);
            } else if(linevec[0].compare("ADD") == 0) {
                actions[i].change[fact] = 1;
            } else if(linevec[0].compare("DEL") == 0) {
                actions[i].change[fact] = -1;
            } else {
                print_parsing_error_and_exit(line, "PRE/ADD/DEL");
            }
            getline(in, line);
        }
    }
    getline(in, line);
    if(line.compare("end_actions") != 0) {
        print_parsing_error_and_exit(line, "end_actions");
    }
    assert(!getline(in, line));

    /*fact_names.push_back("Atom_airborne(airplane_daewh_seg_rwe_0_50)");
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
    fact_names.push_back("Atom_test5");*/
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
