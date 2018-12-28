#include "setformulahorn.h"

#include <fstream>
#include <deque>
#include <algorithm>
#include <cassert>
#include <iostream>

#include "global_funcs.h"

HornUtil::HornUtil(Task *task)
    : task(task) {
    int varamount = task->get_number_of_facts();
    std::vector<std::pair<std::vector<int>,int>> clauses;
    // this is the maximum amount of clauses the formulas need
    clauses.reserve(varamount*2);

    trueformula = new SetFormulaHorn(clauses, varamount);
    clauses.push_back(std::make_pair(std::vector<int>(),-1));
    emptyformula = new SetFormulaHorn(clauses, varamount);
    clauses.clear();

    // insert goal
    const Cube &goal = task->get_goal();
    for(int i = 0; i < goal.size(); ++i) {
        if(goal.at(i) == 1) {
            clauses.push_back(std::make_pair(std::vector<int>(),i));
        }
    }
    goalformula = new SetFormulaHorn(clauses, varamount);
    clauses.clear();

    // insert initial state
    clauses.clear();
    const Cube &init = task->get_initial_state();
    for(int i = 0; i < init.size(); ++i) {
        if(init.at(i) == 1) {
            clauses.push_back(std::make_pair(std::vector<int>(),i));
        } else {
            clauses.push_back(std::make_pair(std::vector<int>(1,i),-1));
        }
    }
    initformula = new SetFormulaHorn(clauses, varamount);
}

void HornUtil::build_actionformulas() {
    int varamount = task->get_number_of_facts();
    std::vector<std::pair<std::vector<int>,int>> clauses;

    actionformulas.reserve(task->get_number_of_actions());
    for(int actionsize = 0; actionsize < task->get_number_of_actions(); ++actionsize) {
        clauses.clear();
        const Action & action = task->get_action(actionsize);
        std::vector<bool> in_pre(varamount,false);
        for(int i = 0; i < action.pre.size(); ++i) {
            clauses.push_back(std::make_pair(std::vector<int>(),action.pre[i]));
            in_pre[action.pre[i]] = true;
        }
        for(int i = 0; i < varamount; ++i) {
            // add effect
            if(action.change[i] == 1) {
                clauses.push_back(std::make_pair(std::vector<int>(),i+varamount));
            // delete effect
            } else if(action.change[i] == -1) {
                clauses.push_back(std::make_pair(std::vector<int>(1,i+varamount),-1));
            // no change
            } else {
                // if var i is in pre but does not change, it must be true after the action application
                if(in_pre[i]) {
                    clauses.push_back(std::make_pair(std::vector<int>(), i+varamount));
                // if var i is not in pre, we need to use frame axioms
                } else {
                    clauses.push_back(std::make_pair(std::vector<int>(1,i),i+varamount));
                    clauses.push_back(std::make_pair(std::vector<int>(1,i+varamount),i));
                }
            }
        }
        actionformulas.push_back(new SetFormulaHorn(clauses,varamount*2));
    }
}

bool HornUtil::get_horn_vector(std::vector<SetFormula *> &formulas, std::vector<SetFormulaHorn *> &horn_formulas) {
    assert(horn_formulas.empty());
    horn_formulas.reserve(formulas.size());
    for(size_t i = 0; i < formulas.size(); ++i) {
        if (formulas[i]->get_formula_type() == SetFormulaType::CONSTANT) {
            SetFormulaConstant *c_formula = static_cast<SetFormulaConstant *>(formulas[i]);
            // see if we can use get_constant_formula
            switch (c_formula->get_constant_type()) {
            case ConstantType::EMPTY:
                horn_formulas.push_back(emptyformula);
                break;
            case ConstantType::GOAL:
                horn_formulas.push_back(goalformula);
                break;
            case ConstantType::INIT:
                horn_formulas.push_back(initformula);
                break;
            default:
                std::cerr << "Unknown constant type " << std::endl;
                exit_with(ExitCode::CRITICAL_ERROR);
                break;
            }
        } else if(formulas[i]->get_formula_type() == SetFormulaType::HORN) {
            SetFormulaHorn *h_formula = static_cast<SetFormulaHorn *>(formulas[i]);
            horn_formulas.push_back(h_formula);
        } else {
            std::cerr << "Error: SetFormula of type other than Horn not allowed here."
                      << std::endl;
            return false;
        }
    }
    return true;
}

// returns false if the unit clause is in conflict of the partial assignment
inline bool is_in_conflict(int var, bool assignment, const Cube &partial_assignment) {
    if((partial_assignment[var] == 0 && assignment) ||
            (partial_assignment[var] == 1 && !assignment)) {
        return true;
    }
    return false;
}

inline bool is_already_queued(int var, bool assignment, const Cube &partial_assignment) {
    return (partial_assignment[var] == 0 && !assignment) ||
           (partial_assignment[var] == 1 && assignment);
}

bool HornUtil::simplify_conjunction(std::vector<HornConjunctionElement> &conjuncts, Cube &partial_assignment) {
    int total_varamount = 0;
    int amount_implications = 0;
    std::vector<int> implstart(conjuncts.size(),-1);
    std::vector<int> primeshift(conjuncts.size(),0);

    std::deque<std::pair<int,bool>> unit_clauses;
    // local means they are not primeshifted
    std::vector<int> leftcount, rightvars_local;
    leftcount.reserve(amount_implications);
    rightvars_local.reserve(amount_implications);

    for(size_t var = 0; var < partial_assignment.size(); ++var) {
        if (partial_assignment[var] == 0) {
            unit_clauses.push_back(std::make_pair(var,false));
        } else if (partial_assignment[var] == 1) {
            unit_clauses.push_back(std::make_pair(var,true));
        }
    }

    for(size_t i = 0; i < conjuncts.size(); ++i) {
        HornConjunctionElement &conjunct = conjuncts[i];
        int local_varamount = conjunct.formula->get_varamount();
        if (conjunct.primed) {
            primeshift[i] = local_varamount;
            local_varamount *= 2;
        }
        total_varamount = std::max(total_varamount, local_varamount);
        partial_assignment.resize(total_varamount,2);
        implstart[i] = amount_implications;
        amount_implications += conjunct.formula->get_size();

        leftcount.insert(leftcount.end(), conjunct.formula->get_left_sizes().begin(),
                         conjunct.formula->get_left_sizes().end());
        rightvars_local.insert(rightvars_local.end(), conjunct.formula->get_right_sides().begin(),
                        conjunct.formula->get_right_sides().end());

        for(int var : conjunct.formula->get_forced_false()) {
            var += primeshift[i];
            if(is_in_conflict(var, false, partial_assignment)) {
                return false;
            } else if (is_already_queued(var, false, partial_assignment)) {
                continue;
            }
            unit_clauses.push_back(std::make_pair(var, false));
            partial_assignment[var] = 0;
        }
        for(int var : conjunct.formula->get_forced_true()) {
            var += primeshift[i];
            if(is_in_conflict(var, true, partial_assignment)) {
                return false;
            } else if (is_already_queued(var, true, partial_assignment)) {
                continue;
            }
            unit_clauses.push_back(std::make_pair(var, true));
            partial_assignment[var] = 1;
        }
        for (size_t impl = 0; impl < conjunct.formula->get_size(); ++impl) {
            if (conjunct.formula->get_left(impl) == 0) {
                int var = conjunct.formula->get_right(impl) + primeshift[i];
                if (var-primeshift[i] == -1 || is_in_conflict(var, true, partial_assignment)) {
                    return false;
                }
                conjunct.removed_implications.push_back(impl);
                if (is_already_queued(var, true, partial_assignment)) {
                    continue;
                }
                unit_clauses.push_back(std::make_pair(var,true));
                partial_assignment[var] = 1;
            } else if (conjunct.formula->get_left(impl) == 1 && conjunct.formula->get_right(impl) == -1) {
                int var = conjunct.formula->get_left_vars(impl)[0] + primeshift[i];
                if (is_in_conflict(var, false, partial_assignment)) {
                    return false;
                }
                conjunct.removed_implications.push_back(impl);
                if (is_already_queued(var, false, partial_assignment)) {
                    continue;
                }
                unit_clauses.push_back(std::make_pair(var,false));
                partial_assignment[var] = 0;
            }
        }
    }

    // unit propagation
    while(!unit_clauses.empty()) {
        std::pair<int,bool> unit_clause = unit_clauses.front();
        unit_clauses.pop_front();

        int var = unit_clause.first;
        if ((partial_assignment[var] == 0 && !unit_clause.second) || (partial_assignment[var] == 1 && unit_clause.second)) {
            continue;
        } else {
            partial_assignment[var] == unit_clause.second ? 1 : 0;
        }

        // positive literal
        if(unit_clause.second) {
            for (size_t i = 0; i < conjuncts.size(); ++i) {
                HornConjunctionElement &conjunct = conjuncts[i];
                auto left_occurences = conjunct.formula->get_variable_occurence_left(var-primeshift[i]);
                auto right_occurences = conjunct.formula->get_variable_occurence_right(var-primeshift[i]);
                for (int left_occ : left_occurences) {
                    leftcount[implstart[i]+left_occ]--;
                    // deleted last negative literal - can only occur if there is a positive literal
                    if (leftcount[implstart[i]+left_occ] == 0) {
                        int newvar = rightvars_local[implstart[i]+left_occ]+primeshift[i];
                        assert(newvar != -1+primeshift[i]);
                        if (is_in_conflict(newvar, true, partial_assignment)) {
                            return false;
                        }  else if (is_already_queued(newvar, true, partial_assignment)) {
                            continue;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, true));
                        partial_assignment[newvar] = 1;
                    // deleted second last negative literal and no positive literal
                    } else if (leftcount[implstart[i]+left_occ] == 1 && rightvars_local[implstart[i]+left_occ] == -1) {
                        // find corresponding variable in left (the only one not set by Cube)
                        int newvar = -1;
                        for (int local_leftvar : conjunct.formula->get_left_vars(left_occ)) {
                            if (partial_assignment[local_leftvar+primeshift[i]] == 2) {
                                newvar = local_leftvar+primeshift[i];
                                break;
                            }
                        }
                        assert(newvar != -1);
                        if (is_in_conflict(newvar, false, partial_assignment)) {
                            return false;
                        }  else if (is_already_queued(newvar, false, partial_assignment)) {
                            continue;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, false));
                        partial_assignment[newvar] = 0;
                    }
                }
                for(int right_occ: right_occurences) {
                    conjunct.removed_implications.push_back(right_occ);
                }
            } // end iterate over formulas
        // negative literal
        } else {
            for (size_t i = 0; i < conjuncts.size(); ++i) {
                HornConjunctionElement &conjunct = conjuncts[i];
                auto left_occurences = conjunct.formula->get_variable_occurence_left(var-primeshift[i]);
                auto right_occurences = conjunct.formula->get_variable_occurence_right(var-primeshift[i]);
                for (int left_occ : left_occurences) {
                    conjunct.removed_implications.push_back(left_occ);
                }
                for(int right_occ: right_occurences) {
                    if(leftcount[implstart[i]+right_occ] == 1) {
                        // find corresponding variable in left (the only one not set by Cube)
                        int newvar = -1;
                        for (int local_leftvar : conjunct.formula->get_left_vars(right_occ)) {
                            if (partial_assignment[local_leftvar+primeshift[i]] == 2) {
                                newvar = local_leftvar+primeshift[i];
                                break;
                            }
                        }
                        assert(newvar != -1);
                        if (is_in_conflict(newvar, false, partial_assignment)) {
                            return false;
                        }  else if (is_already_queued(newvar, false, partial_assignment)) {
                            continue;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, false));
                        partial_assignment[newvar] = 0;
                    }
                }
            } // end iterate over formulas
        } // end if else (positive or negative unit clause)
    } // end while unit clauses not empty
    return true;
}

bool HornUtil::is_restricted_satisfiable(const SetFormulaHorn *formula, Cube &restriction) {
    std::vector<HornConjunctionElement> vec(1,HornConjunctionElement(formula,false));
    return simplify_conjunction(vec, restriction);
}


inline bool update_current_clauses(std::vector<int> &current_clauses, std::vector<int> &clause_amount) {
    int pos_to_change = 0;
    while (current_clauses[pos_to_change] == clause_amount[pos_to_change]-1) {
        current_clauses[pos_to_change] = 0;
        pos_to_change++;
        if (pos_to_change == current_clauses.size()) {
            return false;
        }
    }
    current_clauses[pos_to_change]++;
    return true;
}


bool HornUtil::conjunction_implies_disjunction(std::vector<HornConjunctionElement> &conjuncts,
                                               std::vector<HornDisjunctionElement> &disjuncts) {

    if (disjuncts.size() == 0) {
        Cube partial_assignment;
        return !simplify_conjunction(conjuncts, partial_assignment);
    } else if (conjuncts.size() == 0) {
        conjuncts.push_back(HornConjunctionElement(trueformula, false));
    }
    std::vector<int> clause_amounts;
    clause_amounts.reserve(disjuncts.size());
    int disj_varamount = 0;
    std::vector<int> current_clauses(disjuncts.size(), 0);
    std::vector<int> primeshift(disjuncts.size(), 0);
    for (size_t i = 0; i < disjuncts.size(); ++i) {
        HornDisjunctionElement elem = disjuncts[i];
        /*
         * An empty formula is equivalent to \top
         * -> return true since everything is a subset of a union containing \top
         */
        if (elem.formula->get_size() == 0) {
            return true;
        }
        clause_amounts.push_back(elem.formula->get_forced_true().size()
                                 +elem.formula->get_forced_false().size()
                                 +elem.formula->get_size());
        int local_varamount = elem.formula->get_varamount();
        if (elem.primed) {
            primeshift[i] = local_varamount;
            local_varamount *= 2;
        }
        disj_varamount = std::max(disj_varamount, local_varamount);
    }

    SetFormulaHorn conjunction_formula(conjuncts);

    // try for each clause combination if its negation together with conjuncts is unsat
    do {
        bool unsatisfiable = false;
        Cube partial_assignment(disj_varamount, 2);
        for (size_t i = 0; i < current_clauses.size(); ++i) {
            const SetFormulaHorn *formula = disjuncts[i].formula;
            int clausenum = current_clauses[i];
            if (clausenum < formula->get_forced_true().size()) {
                int forced_true = formula->get_forced_true()[i];
                if (partial_assignment[forced_true+primeshift[i]] == 1) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[forced_true+primeshift[i]] = 0;
            }

            clausenum -= formula->get_forced_true().size();
            if (clausenum >= 0 && clausenum < formula->get_forced_false().size()) {
                int forced_false = formula->get_forced_false()[i];
                if (partial_assignment[forced_false+primeshift[i]] == 0) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[forced_false+primeshift[i]] = 1;
            }

            clausenum -= formula->get_forced_false().size();
            if (clausenum >= 0) {
                for (int left_var : formula->get_left_vars(clausenum)) {
                    if (partial_assignment[left_var+primeshift[i]] == 0) {
                        unsatisfiable = true;
                        break;
                    }
                    partial_assignment[left_var+primeshift[i]] = 1;
                }
                int right_var = formula->get_right(clausenum);
                if (right_var != -1) {
                    if (partial_assignment[right_var+primeshift[i]] == 1) {
                        unsatisfiable = true;
                        break;
                    }
                    partial_assignment[right_var+primeshift[i]] = 0;
                }
            }

            if (unsatisfiable) {
                break;
            }
        } // end loop over disjuncts

        if (unsatisfiable) {
            continue;
        }
        if (is_restricted_satisfiable(&conjunction_formula, partial_assignment)) {
            conjunction_formula.dump();
            for (size_t i = 0; i < current_clauses.size(); ++i) {
                disjuncts[i].formula->dump();
            }
            return false;
        }
    } while (update_current_clauses(current_clauses, clause_amounts));

    return true;
}

HornUtil *SetFormulaHorn::util = nullptr;

SetFormulaHorn::SetFormulaHorn(Task *task) {
    if (util == nullptr) {
        util = new HornUtil(task);
    }
}

SetFormulaHorn::SetFormulaHorn(const std::vector<std::pair<std::vector<int>, int> > &clauses, int varamount)
    : left_vars(clauses.size()), right_side(clauses.size()), variable_occurences(varamount), varamount(varamount) {

    for(int i = 0; i < clauses.size(); ++i) {
        left_vars[i] = clauses[i].first;
        right_side[i] = clauses[i].second;
        for(int j = 0; j < left_vars[i].size(); ++j) {
            variable_occurences[left_vars[i][j]].first.insert(i);
        }
        if(right_side[i] != -1) {
            variable_occurences[right_side[i]].second.insert(i);
        }
        left_sizes.push_back(left_vars[i].size());
    }
    simplify();
    varorder.reserve(varamount);
    for (size_t var = 0; var < varamount; ++var) {
        varorder.push_back(var);
    }
}

SetFormulaHorn::SetFormulaHorn(std::vector<HornConjunctionElement> &elements) {

    Cube partial_assignments;
    bool satisfiable = util->simplify_conjunction(elements, partial_assignments);
    varamount = partial_assignments.size();
    variable_occurences.resize(varamount);

    if (!satisfiable) {
        left_vars.push_back(std::vector<int>());
        right_side.push_back(-1);
    } else {
        // set forced true/false
        for (size_t i = 0; i < partial_assignments.size(); ++i) {
            if (partial_assignments[i] == 0) {
                forced_false.push_back(i);
            } else if (partial_assignments[i] == 1) {
                forced_true.push_back(i);
            }
        }
        // gather total implication count and sort removed implication indices
        int implication_amount = 0;
        for (HornConjunctionElement elem : elements) {
            implication_amount += elem.formula->get_size()-elem.removed_implications.size();
            std::sort(elem.removed_implications.begin(), elem.removed_implications.end());
        }
        left_sizes.reserve(implication_amount);
        left_vars.reserve(implication_amount);
        right_side.reserve(implication_amount);

        // iterate over all formulas and insert each simplified clause one by one
        for (HornConjunctionElement elem : elements) {
            int ri_index = 0;
            // iterate over all implications of the current formula
            for (int i = 0; i < elem.formula->get_size(); ++i) {
                // implication is removed - jump over it
                if (ri_index < elem.removed_implications.size()
                        && i == elem.removed_implications[ri_index]) {
                    ri_index++;
                    continue;
                }

                std::vector<int> left;
                for (int var : elem.formula->get_left_vars(i)) {
                    // var is assigned - jump over it
                    if (partial_assignments[var] != 2) {
                        continue;
                    }
                    left.push_back(var);
                    variable_occurences[var].first.insert(left_vars.size());
                }
                left_sizes.push_back(left.size());
                left_vars.push_back(std::move(left));

                int right_var = elem.formula->get_right(i);
                // var is assigned - no right var
                if(partial_assignments[right_var] != 2) {
                    right_side.push_back(-1);
                } else {
                    variable_occurences[right_var].second.insert(right_side.size());
                    right_side.push_back(right_var);
                }
            } // end iterate over implications of the current formula
        } // end iterate over all formulas
    } // end else block of !satisfiable
    varorder.reserve(varamount);
    for (size_t var = 0; var < varamount; ++var) {
        varorder.push_back(var);
    }
}

SetFormulaHorn::SetFormulaHorn(std::ifstream &input, Task *task) {

    // parsing
    std::string word;
    int clausenum;
    input >> word;
    if (word.compare("p") != 0) {
        std::cerr << "Invalid DIMACS format" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    input >> word;
    if (word.compare("cnf") != 0) {
        std::cerr << "DIMACS format" << word << "not recognized" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    input >> varamount;
    input >> clausenum;
    left_vars.resize(clausenum);
    left_sizes.resize(clausenum, -1);
    right_side.resize(clausenum, -1);
    variable_occurences.resize(varamount);

    for(int i = 0; i < clausenum; ++i) {
        int var;
        input >> var;
        while(var != 0) {
            if(var < 0) {
                var = -1-var;
                left_vars[i].push_back(var);
                variable_occurences[var].first.insert(i);
            } else {
                var -= 1;
                if(right_side[i] != -1) {
                    std::cerr << "Invalid Horn formula" << std::endl;
                    exit_with(ExitCode::CRITICAL_ERROR);
                }
                right_side[i] = var;
                variable_occurences[var].second.insert(i);
            }
            input >> var;
        }
        left_sizes[i] = left_vars[i].size();
    }
    input >> word;
    if(word.compare(";") != 0) {
        std::cerr << "HornFormula syntax wrong" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    // create util if this is the first horn formula
    if (util == nullptr) {
        util = new HornUtil(task);
    }
    simplify();
    varorder.reserve(varamount);
    for (size_t var = 0; var < varamount; ++var) {
        varorder.push_back(var);
    }
}

void SetFormulaHorn::simplify() {
    // call simplify_conjunction to get a partial assignment and implications to remove
    std::vector<HornConjunctionElement> tmpvec(1,HornConjunctionElement(this, false));
    Cube assignments(varamount, 2);
    bool satisfiable = util->simplify_conjunction(tmpvec, assignments);

    if(!satisfiable) {
        left_vars.clear();
        left_vars.push_back(std::vector<int>());
        right_side.clear();
        right_side.push_back(-1);
        variable_occurences.clear();
        variable_occurences.resize(varamount);
        forced_true.clear();
        forced_false.clear();
    } else {
        // set forced true/false and clear up variable occurences
        forced_true.clear();
        forced_false.clear();
        for (size_t var = 0; var < assignments.size(); ++var) {
            // not assigned
            if (assignments[var] == 2) {
                continue;
            }
            // assigned - remove from implications and erase variable occurences entry
            for(int impl : variable_occurences[var].first) {
                left_sizes[impl]--;
                left_vars[impl].erase(std::remove(left_vars[impl].begin(), left_vars[impl].end(), var), left_vars[impl].end());
            }
            for(int impl : variable_occurences[var].second) {
                right_side[impl] = -1;
            }
            variable_occurences[var].first.clear();
            variable_occurences[var].second.clear();
            if (assignments[var] == 0) {
                forced_false.push_back(var);
            } else if (assignments[var] == 1) {
                forced_true.push_back(var);
            }
        }

        // remove unneeded implications
        std::vector<int> &implications_to_remove = tmpvec[0].removed_implications;
        std::sort(implications_to_remove.begin(), implications_to_remove.end());
        int old_location = right_side.size()-1;
        int biggest_to_move_index = implications_to_remove.size()-1;
        for(size_t i = 0; i < implications_to_remove.size(); ++i) {
            int new_location = implications_to_remove[i];
            //find the implication with the highest index number that is not going to be deleted
            while(implications_to_remove[biggest_to_move_index] == old_location) {
                old_location--;
                biggest_to_move_index--;
            }

            // every implication with index higher than new_location is already 0
            // --> we can break out the loop since removed_impl is sorted, i.e. all
            // removed_impl[j] with j > i will also have higher index than old_location
            if(old_location <= new_location) {
                break;
            }

            // swap the implication from old_location to new_location
            // (since the implication at new_location is empty, we don't need to swap this part)
            right_side[new_location] = right_side[old_location];
            left_vars[new_location] = std::move(left_vars[old_location]);

            // update variable_occurences
            for(size_t j = 0; j < variable_occurences.size(); ++j) {
                if(variable_occurences[j].first.erase(old_location) > 0) {
                    variable_occurences[j].first.insert(new_location);
                }
                if(variable_occurences[j].second.erase(old_location) > 0) {
                    variable_occurences[j].second.insert(new_location);
                }
            }
            old_location--;
        }

        int newsize = right_side.size() - implications_to_remove.size();
        // remove empty implications
        right_side.resize(newsize);
        left_vars.resize(newsize);

    }
}


const std::unordered_set<int> &SetFormulaHorn::get_variable_occurence_left(int var) const {
    return variable_occurences[var].first;
}

const std::unordered_set<int> &SetFormulaHorn::get_variable_occurence_right(int var) const {
    return variable_occurences[var].second;
}

int SetFormulaHorn::get_size() const {
    return left_vars.size();
}

int SetFormulaHorn::get_varamount() const {
    return varamount;
}

int SetFormulaHorn::get_left(int index) const{
    return left_vars[index].size();
}

const std::vector<int> &SetFormulaHorn::get_left_sizes() const {
    return left_sizes;
}

int SetFormulaHorn::get_right(int index) const {
    return right_side[index];
}

const std::vector<int> &SetFormulaHorn::get_right_sides() const {
    return right_side;
}

const std::vector<int> &SetFormulaHorn::get_forced_true() const {
    return forced_true;
}

const std::vector<int> &SetFormulaHorn::get_forced_false() const {
    return forced_false;
}

const std::vector<int> &SetFormulaHorn::get_left_vars(int index) const {
    return left_vars[index];
}

void SetFormulaHorn::dump() const{
    std::cout << "forced true: ";
    for(int i = 0; i < forced_true.size(); ++i) {
        std::cout << forced_true[i] << " ";
    }
    std::cout << ", forced false: ";
    for(int i = 0; i < forced_false.size(); ++i) {
        std::cout << forced_false[i] << " ";
    }
    std::cout << std::endl;
    for(int i = 0; i < left_vars.size(); ++i) {
        for(int j = 0; j < left_vars[i].size(); ++j) {
            std::cout << left_vars[i][j] << " ";
        }
        std::cout << "," << right_side[i] << "|";
    }
    std::cout << std::endl;
    std::cout << "right occurences: ";
    for(int i = 0; i < variable_occurences.size(); ++i) {
        std::cout << "[" << i << ":";
        for(auto elem : variable_occurences[i].second) {
            std::cout << elem << " ";
        }
        std::cout << "] ";
    }
    std::cout << std::endl;
}

bool SetFormulaHorn::is_subset(std::vector<SetFormula *> &left,
                               std::vector<SetFormula *> &right) {

    std::vector<SetFormulaHorn *> horn_formulas_left;
    std::vector<SetFormulaHorn *> horn_formulas_right;
    bool valid_horn_formulas = util->get_horn_vector(left,horn_formulas_left)
            && util->get_horn_vector(right, horn_formulas_right);
    if(!valid_horn_formulas) {
        return false;
    }

    std::vector<HornConjunctionElement> conjunction;
    conjunction.reserve(horn_formulas_left.size());
    for (SetFormulaHorn *formula : horn_formulas_left) {
        conjunction.push_back(HornConjunctionElement(formula,false));
    }

    std::vector<HornDisjunctionElement> disjunction;
    disjunction.reserve(horn_formulas_right.size());
    for (SetFormulaHorn *formula : horn_formulas_right) {
        disjunction.push_back(HornDisjunctionElement(formula, false));
    }

    return util->conjunction_implies_disjunction(conjunction, disjunction);
}

bool SetFormulaHorn::is_subset_with_progression(std::vector<SetFormula *> &left,
                                                std::vector<SetFormula *> &right,
                                                std::vector<SetFormula *> &prog,
                                                std::unordered_set<int> &actions) {
    std::vector<SetFormulaHorn *> horn_formulas_left;
    std::vector<SetFormulaHorn *> horn_formulas_right;
    std::vector<SetFormulaHorn *> horn_formulas_prog;
    bool valid_horn_formulas = util->get_horn_vector(left,horn_formulas_left)
            && util->get_horn_vector(right, horn_formulas_right)
            && util->get_horn_vector(prog, horn_formulas_prog);
    if (!valid_horn_formulas) {
        return false;
    }

    std::vector<HornDisjunctionElement> disjunction;
    disjunction.reserve(horn_formulas_right.size());
    for (SetFormulaHorn *formula : horn_formulas_right) {
        disjunction.push_back(HornDisjunctionElement(formula, true));
    }

    std::vector<HornConjunctionElement> conjunction;
    conjunction.reserve(horn_formulas_prog.size()+horn_formulas_left.size());
    for (SetFormulaHorn *formula : horn_formulas_prog) {
        conjunction.push_back(HornConjunctionElement(formula,false));
    }
    for (SetFormulaHorn *formula : horn_formulas_left) {
        conjunction.push_back(HornConjunctionElement(formula,true));
    }

    SetFormulaHorn left_singular(conjunction);
    conjunction.clear();
    conjunction.reserve(2);
    conjunction.push_back(HornConjunctionElement(&left_singular, false));

    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int action : actions) {
        conjunction.push_back(HornConjunctionElement(util->actionformulas[action], false));
        if (!util->conjunction_implies_disjunction(conjunction, disjunction)) {
            return false;
        }
        conjunction.pop_back();
    }

    return true;
}

bool SetFormulaHorn::is_subset_with_regression(std::vector<SetFormula *> &left,
                                               std::vector<SetFormula *> &right,
                                               std::vector<SetFormula *> &reg,
                                               std::unordered_set<int> &actions) {
    std::vector<SetFormulaHorn *> horn_formulas_left;
    std::vector<SetFormulaHorn *> horn_formulas_right;
    std::vector<SetFormulaHorn *> horn_formulas_reg;
    bool valid_horn_formulas = util->get_horn_vector(left,horn_formulas_left)
            && util->get_horn_vector(right, horn_formulas_right)
            && util->get_horn_vector(reg, horn_formulas_reg);
    if (!valid_horn_formulas) {
        return false;
    }

    std::vector<HornDisjunctionElement> disjunction;
    disjunction.reserve(horn_formulas_right.size());
    for (SetFormulaHorn *formula : horn_formulas_right) {
        disjunction.push_back(HornDisjunctionElement(formula, false));
    }

    std::vector<HornConjunctionElement> conjunction;
    conjunction.reserve(horn_formulas_reg.size()+horn_formulas_left.size());
    for (SetFormulaHorn *formula : horn_formulas_reg) {
        conjunction.push_back(HornConjunctionElement(formula,true));
    }
    for (SetFormulaHorn *formula : horn_formulas_left) {
        conjunction.push_back(HornConjunctionElement(formula,false));
    }

    SetFormulaHorn left_singular(conjunction);
    conjunction.clear();
    conjunction.reserve(2);
    conjunction.push_back(HornConjunctionElement(&left_singular, false));

    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int action : actions) {
        conjunction.push_back(HornConjunctionElement(util->actionformulas[action], false));
        if (!util->conjunction_implies_disjunction(conjunction, disjunction)) {
            return false;
        }
        conjunction.pop_back();
    }

    return true;
}

bool SetFormulaHorn::is_subset_of(SetFormula */*superset*/, bool /*left_positive*/, bool /*right_positive*/) {
    // TODO:implement
    return true;
}


SetFormulaType SetFormulaHorn::get_formula_type() {
    return SetFormulaType::HORN;
}

SetFormulaBasic *SetFormulaHorn::get_constant_formula(SetFormulaConstant *c_formula) {
    switch(c_formula->get_constant_type()) {
    case ConstantType::EMPTY:
        return util->emptyformula;
        break;
    case ConstantType::INIT:
        return util->initformula;
        break;
    case ConstantType::GOAL:
        return util->goalformula;
        break;
    default:
        std::cerr << "Unknown Constant type: " << std::endl;
        return nullptr;
        break;
    }
}


const std::vector<int> &SetFormulaHorn::get_varorder() {
    return varorder;
}

bool SetFormulaHorn::is_contained(const std::vector<bool> &model) const {
    Cube cube(varamount, 2);
    for (size_t i = 0; i < model.size(); ++i) {
        if(model[i]) {
            cube[i] = 1;
        } else {
            cube[i] = 0;
        }
    }
    return util->is_restricted_satisfiable(this, cube);
}

bool SetFormulaHorn::is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant) {
    std::vector<std::pair<std::vector<int>,int>> clauses(vars.size());
    for (size_t i = 0; i < vars.size(); ++i) {
        if(implicant[i]) {
            clauses.push_back(std::make_pair(std::vector<int>(),vars[i]));
        } else {
            clauses.push_back(std::make_pair(std::vector<int>(1,vars[i]),-1));
        }
    }
    SetFormulaHorn implicant_horn(clauses, util->task->get_number_of_facts());
    std::vector<HornConjunctionElement> conj(1,HornConjunctionElement(&implicant_horn, false));
    std::vector<HornDisjunctionElement> disj(1,HornDisjunctionElement(this, false));
    return util->conjunction_implies_disjunction(conj, disj);
}

bool SetFormulaHorn::get_next_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) {
    if(i < forced_true.size()) {
        vars.clear();
        clause.clear();
        vars.push_back(forced_true[i]);
        clause.push_back(true);
        return true;
    }

    i -= forced_true.size();
    if (i < forced_false.size()) {
        vars.clear();
        clause.clear();
        vars.push_back(forced_false[i]);
        clause.push_back(false);
        return true;
    }

    i -= forced_false.size();
    if (i < left_vars.size()) {
        vars.clear();
        clause.clear();
        for (int var : left_vars[i]) {
            vars.push_back(var);
            clause.push_back(false);
        }
        if (right_side[i] != -1) {
            vars.push_back(right_side[i]);
            clause.push_back(true);
        }
        return true;

    }
    return false;
}

bool SetFormulaHorn::get_next_model(int i, std::vector<bool> &model) {
    std::cerr << "not implemented";
    exit_with(ExitCode::CRITICAL_ERROR);
}

void SetFormulaHorn::setup_model_enumeration() {
    std::cerr << "not implemented";
    exit_with(ExitCode::CRITICAL_ERROR);
}
