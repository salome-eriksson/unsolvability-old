#include "setformulahorn.h"

#include <fstream>
#include <deque>
#include <algorithm>
#include <cassert>
#include <iostream>

#include "global_funcs.h"

HornConjunctionElement::HornConjunctionElement (const SetFormulaHorn *formula, bool primed)
    : formula(formula), primed(primed), removed_implications(formula->get_size(), false) {
}

HornUtil::HornUtil(Task *task)
    : task(task) {
    int varamount = task->get_number_of_facts();
    std::vector<std::pair<std::vector<int>,int>> clauses;
    // this is the maximum amount of clauses the formulas need
    clauses.reserve(varamount*2);

    trueformula = new SetFormulaHorn(clauses, varamount);
    clauses.push_back(std::make_pair(std::vector<int>(1,0),-1));
    clauses.push_back(std::make_pair(std::vector<int>(),0));
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

    int var = 0;
    for(int assignment : partial_assignment) {
        if (assignment == 0) {
            unit_clauses.push_back(std::make_pair(var,false));
        } else if (assignment == 1) {
            unit_clauses.push_back(std::make_pair(var,true));
        }
        var++;
    }

    for(size_t i = 0; i < conjuncts.size(); ++i) {
        HornConjunctionElement &conjunct = conjuncts[i];
        int local_varamount = conjunct.formula->get_varamount();
        if (conjunct.primed) {
            primeshift[i] = local_varamount;
            local_varamount *= 2;
        }
        total_varamount = std::max(total_varamount, local_varamount);
        implstart[i] = amount_implications;
        amount_implications += conjunct.formula->get_size();
        partial_assignment.resize(total_varamount,2);
        leftcount.insert(leftcount.end(), conjunct.formula->get_left_sizes().begin(),
                         conjunct.formula->get_left_sizes().end());
        rightvars_local.insert(rightvars_local.end(), conjunct.formula->get_right_sides().begin(),
                        conjunct.formula->get_right_sides().end());

        for(int var : conjunct.formula->get_forced_false()) {
            var += primeshift[i];
            if (partial_assignment[var] == 1) {
                return false;
            }
            unit_clauses.push_back(std::make_pair(var, false));
            partial_assignment[var] = 0;
        }
        for(int var : conjunct.formula->get_forced_true()) {
            var += primeshift[i];
            if (partial_assignment[var] == 0) {
                return false;
            }
            unit_clauses.push_back(std::make_pair(var, true));
            partial_assignment[var] = 1;
        }
    }

    std::vector<bool> already_seen(total_varamount, false);

    // unit propagation
    while(!unit_clauses.empty()) {
        std::pair<int,bool> unit_clause = unit_clauses.front();
        unit_clauses.pop_front();

        int var = unit_clause.first;
        if (already_seen[var]) {
            continue;
        }
        int positive = unit_clause.second;
        already_seen[var] = true;
        // positive literal
        if(positive) {
            for (size_t i = 0; i < conjuncts.size(); ++i) {
                HornConjunctionElement &conjunct = conjuncts[i];
                if(var-primeshift[i] < 0 || var-primeshift[i] >= conjunct.formula->get_varamount()) {
                    continue;
                }
                const std::forward_list<int> &left_occurences = conjunct.formula->get_variable_occurence_left(var-primeshift[i]);
                const std::forward_list<int> &right_occurences = conjunct.formula->get_variable_occurence_right(var-primeshift[i]);
                for (int left_occ : left_occurences) {
                    int global_impl = implstart[i]+left_occ;
                    leftcount[global_impl]--;
                    // deleted last negative literal --> must have positive literal
                    if (leftcount[global_impl] == 0) {
                        int newvar = conjunct.formula->get_right(left_occ)+primeshift[i];
                        if (newvar = -1+primeshift[i]
                                || is_in_conflict(newvar, true, partial_assignment)) {
                            return false;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, true));
                        partial_assignment[newvar] = 1;
                        conjunct.removed_implications[left_occ] = true;
                    // deleted second last negative literal and no positive literal
                    } else if (leftcount[global_impl] == 1
                               && rightvars_local[global_impl] == -1) {
                        // find corresponding variable in left
                        int newvar = -1;
                        for (int local_leftvar : conjunct.formula->get_left_vars(left_occ)) {
                            if (partial_assignment[local_leftvar+primeshift[i]] != 1) {
                                newvar = local_leftvar+primeshift[i];
                                break;
                            }
                        }
                        if (newvar == -1) {
                            return false;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, false));
                        partial_assignment[newvar] = 0;
                        conjunct.removed_implications[left_occ] = true;
                    }
                }
                for(int right_occ: right_occurences) {
                    conjunct.removed_implications[right_occ] = true;
                }
            } // end iterate over formulas
        // negative literal
        } else {
            for (size_t i = 0; i < conjuncts.size(); ++i) {
                HornConjunctionElement &conjunct = conjuncts[i];
                if(var-primeshift[i] < 0 || var-primeshift[i] >= conjunct.formula->get_varamount()) {
                    continue;
                }
                const std::forward_list<int> &left_occurences = conjunct.formula->get_variable_occurence_left(var-primeshift[i]);
                const std::forward_list<int> &right_occurences = conjunct.formula->get_variable_occurence_right(var-primeshift[i]);
                for (int left_occ : left_occurences) {
                    conjunct.removed_implications[left_occ] = true;
                }
                for(int right_occ: right_occurences) {
                    rightvars_local[implstart[i]+right_occ] = -1;
                    if(leftcount[implstart[i]+right_occ] == 1) {
                        // find corresponding variable in left
                        int newvar = -1;
                        for (int local_leftvar : conjunct.formula->get_left_vars(right_occ)) {
                            if (partial_assignment[local_leftvar+primeshift[i]] != 1) {
                                newvar = local_leftvar+primeshift[i];
                                break;
                            }
                        }
                        if (newvar == -1) {
                            return false;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, false));
                        partial_assignment[newvar] = 0;
                        conjunct.removed_implications[right_occ] = true;
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

    if (conjuncts.size() == 0) {
        conjuncts.push_back(HornConjunctionElement(trueformula, false));
    }
    std::vector<int> clause_amounts;
    std::vector<int> current_clauses;
    std::vector<int> primeshift;
    int disj_varamount = 0;
    // iterator for removing while iterating
    std::vector<HornDisjunctionElement>::iterator disjunct_it = disjuncts.begin();
    while (disjunct_it != disjuncts.end()) {
        HornDisjunctionElement elem = *disjunct_it;
        /*
         * An empty formula is equivalent to \top
         * -> return true since everything is a subset of a union containing \top
         */
        if (elem.formula->get_size()
                + elem.formula->get_forced_false().size()
                + elem.formula->get_forced_true().size()
                == 0) {
            return true;
        }
        // formula not satisfiable -> ignore it
        Cube dummy;
        if (!is_restricted_satisfiable(elem.formula, dummy)) {
            disjunct_it = disjuncts.erase(disjunct_it);
            continue;
        }
        clause_amounts.push_back(elem.formula->get_forced_true().size()
                                 +elem.formula->get_forced_false().size()
                                 +elem.formula->get_size());
        current_clauses.push_back(0);
        int local_varamount = elem.formula->get_varamount();
        if (elem.primed) {
            primeshift.push_back(local_varamount);
            local_varamount *= 2;
        } else {
            primeshift.push_back(0);
        }
        disj_varamount = std::max(disj_varamount, local_varamount);
        disjunct_it++;
    }

    SetFormulaHorn conjunction_dummy(task);
    const SetFormulaHorn *conjunction = conjuncts[0].formula;
    if(conjuncts.size() > 1) {
        conjunction_dummy = SetFormulaHorn(conjuncts);
        conjunction = &conjunction_dummy;
    }

    Cube conjunction_pa;
    if ( !is_restricted_satisfiable(conjunction,conjunction_pa) ) {
        return true;
    }
    conjunction_pa.resize(disj_varamount, 2);

    // try for each clause combination if its negation together with conjuncts is unsat
    do {
        bool unsatisfiable = false;
        Cube partial_assignment(disj_varamount,2);
        for (size_t i = 0; i < current_clauses.size(); ++i) {
            const SetFormulaHorn *formula = disjuncts[i].formula;
            int clausenum = current_clauses[i];
            if (clausenum < formula->get_forced_true().size()) {
                int forced_true = formula->get_forced_true().at(clausenum);
                if (partial_assignment[forced_true+primeshift[i]] == 1
                        || conjunction_pa[forced_true+primeshift[i]] == 1) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[forced_true+primeshift[i]] = 0;
            }

            clausenum -= formula->get_forced_true().size();
            if (clausenum >= 0 && clausenum < formula->get_forced_false().size()) {
                int forced_false = formula->get_forced_false().at(clausenum);
                if (partial_assignment[forced_false+primeshift[i]] == 0
                        || conjunction_pa[forced_false+primeshift[i]] == 0) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[forced_false+primeshift[i]] = 1;
            }

            clausenum -= formula->get_forced_false().size();
            if (clausenum >= 0) {
                for (int left_var : formula->get_left_vars(clausenum)) {
                    if (partial_assignment[left_var+primeshift[i]] == 0
                            || conjunction_pa[left_var+primeshift[i]] == 0) {
                        unsatisfiable = true;
                        break;
                    }
                    partial_assignment[left_var+primeshift[i]] = 1;
                }
                int right_var = formula->get_right(clausenum);
                if (right_var != -1) {
                    if (partial_assignment[right_var+primeshift[i]] == 1
                            || conjunction_pa[right_var+primeshift[i]] == 1) {
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
        if (is_restricted_satisfiable(conjunction, partial_assignment)) {
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
    : variable_occurences(varamount), varamount(varamount) {

    for(auto clause : clauses) {
        if (clause.first.size() == 0) {
            assert(clause.second != -1);
            forced_true.push_back(clause.second);
            continue;
        } else if (clause.first.size() == 1 && clause.second == -1) {
            forced_false.push_back(clause.first[0]);
            continue;
        }
        left_vars.push_back(clause.first);
        right_side.push_back(clause.second);
        for(int var : clause.first) {
            variable_occurences[var].first.push_front(left_vars.size()-1);
        }
        if(clause.second != -1) {
            variable_occurences[clause.second].second.push_front(left_vars.size()-1);
        }
        left_sizes.push_back(clause.first.size());
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
        variable_occurences.clear();
        variable_occurences.resize(varamount);
        forced_true.clear();
        forced_true.push_back(0);
        forced_false.clear();
        forced_false.push_back(0);
    } else {
        // set forced true/false
        for (size_t i = 0; i < partial_assignments.size(); ++i) {
            if (partial_assignments[i] == 0) {
                forced_false.push_back(i);
            } else if (partial_assignments[i] == 1) {
                forced_true.push_back(i);
            }
        }
        int implication_amount = 0;
        for (HornConjunctionElement elem : elements) {
            for (bool val : elem.removed_implications) {
                if (!val) {
                    implication_amount++;
                }
            }
        }
        left_sizes.reserve(implication_amount);
        left_vars.reserve(implication_amount);
        right_side.reserve(implication_amount);

        // iterate over all formulas and insert each simplified clause one by one
        for (HornConjunctionElement elem : elements) {
            // iterate over all implications of the current formula
            for (int i = 0; i < elem.formula->get_size(); ++i) {
                // implication is removed - jump over it
                if (elem.removed_implications[i] ) {
                    continue;
                }
                std::vector<int> left;
                for (int var : elem.formula->get_left_vars(i)) {
                    if (elem.primed) {
                        var += elem.formula->get_varamount();
                    }
                    // var is assigned - jump over it
                    if (partial_assignments[var] != 2) {
                        continue;
                    }
                    left.push_back(var);
                    variable_occurences[var].first.push_front(left_vars.size());
                }
                left_sizes.push_back(left.size());
                left_vars.push_back(std::move(left));

                int right_var = elem.formula->get_right(i);
                if (right_var == -1) {
                    right_side.push_back(-1);
                    continue;
                }
                // right var is assinged in origin formula
                if (elem.primed) {
                    right_var += elem.formula->get_varamount();
                }
                // right var is assigned in partial assignment - no right var
                if(partial_assignments[right_var] != 2) {
                    right_side.push_back(-1);
                } else {
                    variable_occurences[right_var].second.push_front(right_side.size());
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
    variable_occurences.resize(varamount);

    int count = 0;
    for(int i = 0; i < clausenum; ++i) {
        int var;
        input >> var;
        std::vector<int> left;
        int right = -1;
        while(var != 0) {
            if (var < 0) {
                left.push_back(-1-var);
            } else {
                if (right != -1) {
                    std::cerr << "Invalid Horn formula" << std::endl;
                    exit_with(ExitCode::CRITICAL_ERROR);
                }
                right = var-1;
            }
            input >> var;
        }
        if (left.size() == 0) {
            if (right == -1) {
                std::cerr << "Invalid Horn formula" << std::endl;
                exit_with(ExitCode::CRITICAL_ERROR);
            }
            forced_true.push_back(right);
        } else if (left.size() == 1 && right == -1) {
            forced_false.push_back(left[0]);
        } else {
            for (int var : left) {
                variable_occurences[var].first.push_front(count);
            }
            if (right != -1) {
                variable_occurences[var].second.push_front(count);
            }
            left_sizes.push_back(left.size());
            left_vars.push_back(std::move(left));
            right_side.push_back(right);
            count++;
        }
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
        right_side.clear();
        variable_occurences.clear();
        variable_occurences.resize(varamount);
        forced_true.clear();
        forced_true.push_back(0);
        forced_false.clear();
        forced_false.push_back(0);
    } else {
        // set forced true/false and clear up variable occurences
        forced_true.clear();
        forced_false.clear();
        for (size_t var = 0; var < assignments.size(); ++var) {
            // not assigned
            if (assignments[var] == 2) {
                continue;
            }
            // assigned - remove from implications
            for(int impl : variable_occurences[var].first) {
                left_sizes[impl]--;
                left_vars[impl].erase(std::remove(left_vars[impl].begin(),
                                                  left_vars[impl].end(), var),
                                      left_vars[impl].end());
            }
            for(int impl : variable_occurences[var].second) {
                right_side[impl] = -1;
            }
            if (assignments[var] == 0) {
                forced_false.push_back(var);
            } else if (assignments[var] == 1) {
                forced_true.push_back(var);
            }
        }

        // remove unneeded implications
        std::vector<bool> &implications_to_remove = tmpvec[0].removed_implications;
        int front = 0;
        int back = implications_to_remove.size()-1;

        while (front < back) {
            // back points to the last implication that should not be removed
            while (front < back && implications_to_remove[back]) {
                back--;
            }
            // front points to the next implication to remove
            while (front < back &&!implications_to_remove[front]) {
                front++;
            }
            if (front >= back) {
                break;
            }
            // move the implication at back to front
            right_side[front] = right_side[back];
            left_vars[front] = std::move(left_vars[back]);
            left_sizes[front] = left_sizes[back];
            front++;
            back--;
        }

        int newsize = 0;
        for (bool val : implications_to_remove) {
            if (!val) {
                newsize++;
            }
        }
        right_side.resize(newsize);
        left_vars.resize(newsize);
        left_sizes.resize(newsize);
        right_side.shrink_to_fit();
        left_vars.shrink_to_fit();
        left_sizes.shrink_to_fit();

    }
    variable_occurences.clear();
    variable_occurences.resize(varamount);
    for (size_t i = 0; i < left_vars.size(); ++i) {
        for (int var : left_vars[i]) {
            variable_occurences[var].first.push_front(i);
        }
        int var = right_side[i];
        if (var != -1) {
            variable_occurences[var].second.push_front(i);
        }
    }
}


const std::forward_list<int> &SetFormulaHorn::get_variable_occurence_left(int var) const {
    return variable_occurences[var].first;
}

const std::forward_list<int> &SetFormulaHorn::get_variable_occurence_right(int var) const {
    return variable_occurences[var].second;
}

int SetFormulaHorn::get_size() const {
    return left_vars.size();
}

int SetFormulaHorn::get_varamount() const {
    return varamount;
}

int SetFormulaHorn::get_left(int index) const{
    return left_sizes[index];
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
        std::cout << "," << right_side[i] << "(" << left_sizes[i] << ")"<< "|";
    }
    std::cout << std::endl;
    std::cout << "occurences: ";
    for(int i = 0; i < variable_occurences.size(); ++i) {
        std::cout << "[" << i << ":";
        for(auto elem : variable_occurences[i].first) {
            std::cout << elem << " ";
        }
        std::cout << "|";
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

    SetFormulaHorn left_dummy(util->task);
    if (conjunction.size() > 1) {
        left_dummy = SetFormulaHorn(conjunction);
        conjunction.clear();
        conjunction.push_back(HornConjunctionElement(&left_dummy,false));
    }
    // conjunction now only contains 1 element
    conjunction.reserve(2);

    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int action : actions) {
        conjunction[0].removed_implications.clear();
        conjunction[0].removed_implications.resize(conjunction[0].formula->get_size(), false);
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
