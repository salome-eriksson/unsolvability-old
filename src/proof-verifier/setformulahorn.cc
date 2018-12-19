#include "setformulahorn.h"

#include <fstream>
//#include <stack>
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

/*bool HornUtil::is_satisfiable(const HornFormulaList &formulas, bool partial) {
    Cube restrictions, solution;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}

bool HornUtil::is_satisfiable(const HornFormulaList &formulas, Cube &solution,
                                          bool partial) {
    Cube restrictions;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}

bool HornUtil::is_restricted_satisfiable(const HornFormulaList &formulas,
                                                     const Cube &restrictions, bool partial) {
    Cube solution;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}


bool HornUtil::is_restricted_satisfiable(const HornFormulaList &formulas,
                                                     const Cube &restrictions, Cube &solution,
                                                     bool partial) {
    assert(formulas.size() >= 1);
    int varamount = formulas[0].first->get_varamount();
    // total amount of implications
    int implamount = 0;
    // the index for which the implications for this formula start in left_count
    std::vector<int> implstart(formulas.size());

    // count amount of implication and total varamount
    for(size_t i = 0; i < formulas.size(); ++i) {
        implstart[i] = implamount;
        int localvaramount = formulas[i].first->get_varamount();
        if(formulas[i].second) {
            localvaramount *= 2;
        }
        varamount = std::max(varamount, localvaramount);
        implamount += formulas[i].first->get_size();

        const SetFormulaHorn *f = formulas[i].first;
        if(f->get_size() == 1 && f->get_left(0) == 0 && f->get_right(0) == -1) {
            return false;
        }
    }

    Cube local_restrictions(restrictions);
    local_restrictions.resize(varamount,2);

    // set forced true/false vars in the local restriction cube
    for(auto fentry : formulas) {
        int offset = 0;
        if(fentry.second) {
            offset = fentry.first->get_varamount();
        }
        for(int ff : fentry.first->get_forced_false()) {
            if(local_restrictions[ff+offset] == 1) {
                return false;
            } else {
                local_restrictions[ff+offset] = 0;
            }
        }
        for(int ft : fentry.first->get_forced_true()) {
            if(local_restrictions[ft+offset] == 0) {
                return false;
            } else {
                local_restrictions[ft+offset] = 1;
            }
        }
    }

    std::stack<int> forced_true;
    for(int i = 0; i < varamount; ++i) {
        if(local_restrictions[i] == 1) {
            forced_true.push(i);
        }
    }

    // setting up left size count for implications
    // left count cannot be 0 anywhere since we simplify the formulas by construction time
    std::vector<int> left_count;
    left_count.reserve(implamount);
    for(auto fentry : formulas) {
        const SetFormulaHorn *formula = fentry.first;
        left_count.insert(left_count.end(), formula->get_left_sizes().begin(), formula->get_left_sizes().end());
    }

    solution.resize(varamount);
    std::fill(solution.begin(), solution.end(), 0);

    while(!forced_true.empty()) {
        int var = forced_true.top();
        forced_true.pop();
        if(solution[var] == 1) {
            continue;
        }
        solution[var] = 1;
        for(size_t findex = 0; findex < formulas.size(); ++findex) {
            int local_varamount = formulas[findex].first->get_varamount();
            // if the variable is primed but not the formula or vice versa we don't need to change anything
            if((var >= local_varamount && !formulas[findex].second) ||
               (var < local_varamount && formulas[findex].second)) {
                continue;
            }
            int offset = 0;
            if(formulas[findex].second) {
                offset = local_varamount;
            }
            const SetFormulaHorn *formula = formulas[findex].first;
            const std::unordered_set<int> &var_occurences = formula->get_variable_occurence_left(var-offset);
            for(int internal_impl_number: var_occurences) {
                int impl_number = internal_impl_number + implstart[findex];
                left_count[impl_number]--;
                // right side is forced true
                if(left_count[impl_number] == 0) {
                    int right = formula->get_right(internal_impl_number);
                    if(right == -1 || local_restrictions[right+offset] == 0) {
                        return false;
                    }
                    forced_true.push(right+offset);
                    local_restrictions[right+offset] = 1;
                // last element in left side and no right side --> forced false
                } else if(left_count[impl_number] == 1 &&
                          (formula->get_right(internal_impl_number) == -1 ||
                           local_restrictions[formula->get_right(internal_impl_number)] == 0)) {
                    bool found = false;
                    for(int left_var : formula->get_left_vars(internal_impl_number)) {
                        if(local_restrictions[left_var+offset] != 1) {
                            found = true;
                            local_restrictions[left_var+offset] = 0;
                            break;
                        }
                    }
                    if(!found) {
                        return false;
                    }
                }
            }
        }
    }
    if(partial) {
        std::stack<int> forced_false;
        for(size_t i = 0; i < local_restrictions.size(); ++i) {
            if(local_restrictions[i] == 0) {
                forced_false.push(i);
                local_restrictions[i] = 2;
            }
        }
        for(size_t findex = 0; findex < formulas.size(); ++findex) {
            const SetFormulaHorn *f = formulas[findex].first;
            int offset = 0;
            if(formulas[findex].second) {
                offset += f->get_varamount();
            }
        }
        while(!forced_false.empty()) {
            int var = forced_false.top();
            forced_false.pop();
            if(local_restrictions[var] == 0) {
                continue;
            }
            local_restrictions[var] = 0;
            for(int findex = 0; findex < formulas.size(); ++findex) {
                int local_varamount = formulas[findex].first->get_varamount();
                // if the variable is primed but not the formula or vice versa we don't need to change anything
                if((var >= local_varamount && !formulas[findex].second) ||
                   (var < local_varamount && formulas[findex].second)) {
                    continue;
                }
                int offset = 0;
                if(formulas[findex].second) {
                    offset = local_varamount;
                }
                const SetFormulaHorn *formula = formulas[findex].first;
                const std::unordered_set<int> &var_occurences = formula->get_variable_occurence_right(var-offset);
                for(int internal_impl_number: var_occurences) {
                    int impl_number = internal_impl_number + implstart[findex];
                    if(left_count[impl_number] == 1) {
                        const std::vector<int> left = formula->get_left_vars(internal_impl_number);
                        for(int pos = 0; pos < left.size(); ++ pos) {
                            if(local_restrictions[left[pos]+offset] ==2) {
                                forced_false.push(left[pos]+offset);
                            }
                        }
                    }
                }
            }
        }
        solution.swap(local_restrictions);
    }
    return true;
}


/* If formula1 implies formula2, then formula1 \land \lnot formula 2 is unsatisfiable.
 * Since Horn formulas don't support negation but the negation is a disjunction over
 * unit clauses we can test for each disjunction separately if
 * formula1 \land disjunction is unsatisfiable.
 * special cases:
 *  - if the right formula is empty (= true) then it correctly returns true since we don't enter the loop
 *  - if the right formula contains an empty implication (no left side, right side = -1 --> empty)
 *    we return false only if the formulalist is satisfiable without restrictions (since we do not set any)
 */
/*bool HornUtil::implies(const HornFormulaList &formulas,
                                    const SetFormulaHorn *right, bool right_primed) {
    assert(formulas.size() >= 1);

    Cube left_solution_partial;
    if(!is_satisfiable(formulas, left_solution_partial, true)) {
        return true;
    }

    int right_varamount = right->get_varamount();
    int offset = 0;
    if(right_primed) {
        offset += right_varamount;
        right_varamount *= 2;
    }

    Cube restrictions = Cube(right_varamount,2);
    // first loop over all forced true
    for(size_t i = 0; i < right->get_forced_true().size(); ++ i) {
        int forced_true = right->get_forced_true().at(i)+offset;
        // if left also forces the same variable true, then we don't need to check
        if(left_solution_partial[forced_true] == 1) {
            continue;
        }
        restrictions[forced_true] = 0;
        if(is_restricted_satisfiable(formulas, restrictions)) {
            return false;
        }
        restrictions[forced_true] = 2;
    }

    // then loop over all forced false
    for(size_t i = 0; i < right->get_forced_false().size(); ++i) {
        int forced_false = right->get_forced_false().at(i)+offset;
        if(left_solution_partial[forced_false] == 0) {
            continue;
        }
        // it is still possible that ie right forces \lnot a true, but we don't reach that conclusion
        // with left, e.g. left has the clauses (\lot a \lor b) \land (\lnot a \lor \lnot b)
        restrictions[forced_false] = 1;
        if(is_restricted_satisfiable(formulas, restrictions)) {
            return false;
        }
        restrictions[forced_false] = 2;
    }

    // now loop over implications
    for(size_t i = 0; i < right->get_size(); ++i) {
        const std::vector<int> &left_vars = right->get_left_vars(i);
        /* We want to check for each clause if it is implied. If one variable of the clause
         * is already part of the solution to formulas then it is implied and we don't need
         * to check anything for this clause.

        bool covered = false;
        for(size_t j = 0; j < left_vars.size(); ++j) {
            if(left_solution_partial[left_vars.at(j)+offset] == 0) {
                covered = true;
                break;
            }
            restrictions[left_vars.at(j)+offset] = 1;
        }
        int r = right->get_right(i);
        if(!covered && r != -1) {
            /* First part: var is already directly implied from formulas.
             * Second part: if the current implication is (\lnot a \lor \lnot ... \lor a)
             * it equals true and is thus implied by formulas

            if(left_solution_partial[r+offset] == 1 || restrictions[r+offset] == 1) {
                covered = true;
            } else {
                restrictions[r+offset] = 0;
            }
        }
        if(!covered && is_restricted_satisfiable(formulas, restrictions)) {
            return false;
        }
        std::fill(restrictions.begin(), restrictions.end(),2);
    }
    return true;
}
*/

// Given restrictions for a clause of the left formula, go through all right formulas
/*bool HornUtil::implies_union_helper(const HornFormulaList &formulas, Cube &restrictions,
                          const SetFormulaHorn *right_formula, int right_offset,
                          const Cube &formulas_solution_partial) {
    // right forced true
    for(int ft_right : right_formula->get_forced_true()) {
        if(formulas_solution_partial[ft_right+right_offset] == 1 ||
                restrictions[ft_right+right_offset] == 1) {
            continue;
        }
        restrictions[ft_right+right_offset] = 0;
        if(is_restricted_satisfiable(formulas, restrictions)) {
            return false;
        }
        restrictions[ft_right+right_offset] = 2;
    }

    // right forced false;
    for(int ff_right : right_formula->get_forced_false()) {
        if(formulas_solution_partial[ff_right+right_offset] == 0 ||
                restrictions[ff_right+right_offset] == 0) {
            continue;
        }
        restrictions[ff_right+right_offset] = 1;
        if(is_restricted_satisfiable(formulas, restrictions)) {
            return false;
        }
        restrictions[ff_right+right_offset] = 2;
    }

    // right implications
    // TODO: maybe this copy can be done nicer...
    Cube restrictions_copy = restrictions;
    for(size_t i = 0; i < right_formula->get_size(); ++i) {
        const std::vector<int> &neg_vars = right_formula->get_left_vars(i);
        bool covered = false;
        for(int neg_var : neg_vars) {
            if(formulas_solution_partial[neg_var+right_offset] == 0 ||
                    restrictions[neg_var+right_offset] == 0) {
                covered = true;
                break;
            }
            restrictions[neg_var+right_offset] = 1;
        }
        int r = right_formula->get_right(i);
        if(!covered && r != -1) {
            if(formulas_solution_partial[r+right_offset] == 1 ||
                    restrictions[r+right_offset] == 1) {
                covered = true;
            } else {
                restrictions[r+right_offset] = 0;
            }
        }
        if(!covered && is_restricted_satisfiable(formulas, restrictions)) {
            return false;
        }

        restrictions = restrictions_copy;
    }

    return true;
}

// TODO: needs refactoring
bool HornUtil::implies_union(const HornFormulaList &formulas,
                                  const SetFormulaHorn *union_left, bool left_primed,
                                  const SetFormulaHorn *union_right, bool right_primed) {
    //we currently demand that the union formula parts are "atomic"
    assert(formulas.size() >= 1);

    Cube formulas_solution_partial;
    if(!is_satisfiable(formulas, formulas_solution_partial, true)) {
        return true;
    }

    int left_varamount = union_left->get_varamount();
    int left_offset = 0;
    if(left_primed) {
        left_offset += left_varamount;
        left_varamount *= 2;
    }
    int right_varamount = union_right->get_varamount();
    int right_offset = 0;
    if(right_primed) {
        right_offset += right_varamount;
        right_varamount *= 2;
    }

    Cube restrictions = Cube(std::max(left_varamount, right_varamount),2);
    // left forced true
    for(int ft_left: union_left->get_forced_true()) {
        if (formulas_solution_partial[ft_left+left_offset] == 1) {
            continue;
        }
        restrictions[ft_left+left_offset] = 0;
        if (!implies_union_helper(formulas, restrictions,
                                 union_right, right_offset, formulas_solution_partial)) {
            return false;
        }
        restrictions[ft_left+left_offset] = 2;
    }

    // left forced false
    for(int ff_left: union_left->get_forced_false()) {
        if (formulas_solution_partial[ff_left+left_offset] == 0) {
            continue;
        }
        restrictions[ff_left+left_offset] = 1;
        if (!implies_union_helper(formulas, restrictions,
                                 union_right, right_offset, formulas_solution_partial)) {
            return false;
        }
        restrictions[ff_left+left_offset] = 2;
    }

    // left implications
    for(size_t i = 0; i < union_left->get_size(); ++i) {
        const std::vector<int> &neg_vars = union_left->get_left_vars(i);
        bool covered = false;
        for(int neg_var : neg_vars) {
            if(formulas_solution_partial[neg_var+left_offset] == 0) {
                covered = true;
                break;
            }
            restrictions[neg_var+left_offset] = 1;
        }
        int r = union_left->get_right(i);
        if (!covered && r != -1) {
            std::cout << r << std::endl;
            if (formulas_solution_partial[r+left_offset] == 1 || restrictions[r+left_offset] == 1) {
                covered = true;
            } else {
                restrictions[r+left_offset] = 0;
            }
        }
        if (!covered && !implies_union_helper(formulas, restrictions,
                                     union_right, right_offset, formulas_solution_partial)) {
            return false;
        }
        std::fill(restrictions.begin(), restrictions.end(),2);
    }
    return true;
}
*/

// returns false if the unit clause is in conflict of the partial assignment
inline bool is_in_conflict(int var, bool assignment, const Cube &partial_assignment) {
    if((partial_assignment[var] == 0 && assignment) ||
            (partial_assignment[var] == 1 && !assignment)) {
        return true;
    }
    return false;
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
            partial_assignment[var] = 2;
        } else if (partial_assignment[var] == 1) {
            unit_clauses.push_back(std::make_pair(var,true));
            partial_assignment[var] = 2;
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
            }
            unit_clauses.push_back(std::make_pair(var, false));
            //partial_assignment[var] = 0;
        }
        for(int var : conjunct.formula->get_forced_true()) {
            var += primeshift[i];
            if(is_in_conflict(var, true, partial_assignment)) {
                return false;
            }
            unit_clauses.push_back(std::make_pair(var, true));
            //partial_assignment[var] = 1;
        }
        for (size_t impl = 0; impl < conjunct.formula->get_size(); ++impl) {
            if (conjunct.formula->get_left(impl) == 0) {
                int var = conjunct.formula->get_right(impl) + primeshift[i];
                if (var-primeshift[i] == -1 || is_in_conflict(var, true, partial_assignment)) {
                    return false;
                }
                unit_clauses.push_back(std::make_pair(var,true));
                //partial_assignment[var] = 1;
            } else if (conjunct.formula->get_left(impl) == 1 && conjunct.formula->get_right(impl) == -1) {
                int var = conjunct.formula->get_left_vars(impl)[0] + primeshift[i];
                if (is_in_conflict(var, false, partial_assignment)) {
                    return false;
                }
                unit_clauses.push_back(std::make_pair(var,false));
                //partial_assignment[var] = 0;
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
                        }
                        unit_clauses.push_back(std::make_pair(newvar, true));
                        //partial_assignment[var] = 1;
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
                        }
                        unit_clauses.push_back(std::make_pair(newvar, false));
                        //partial_assignment[var] = 0;
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
                        }
                        unit_clauses.push_back(std::make_pair(newvar, false));
                    }
                }
            } // end iterate over formulas
        } // end if else (positive or negative unit clause)
    } // end while unit clauses not empty
    return true;
}

bool HornUtil::is_restricted_satisfiable(SetFormulaHorn *formula, Cube &restriction) {
    std::vector<HornConjunctionElement> vec(1,HornConjunctionElement(formula,false));
    return simplify_conjunction(vec, restriction);
}


inline bool update_current_clauses(std::vector<int> &current_clauses, std::vector<int> &clause_amount) {
    int pos_to_change = 0;
    while (current_clauses[pos_to_change] == clause_amount[pos_to_change]-1) {
        pos_to_change++;
        if (pos_to_change == current_clauses.size()) {
            return false;
        }
    }
    current_clauses[pos_to_change]++;
    return true;
}


bool HornUtil::conjunction_implies_disjunction(std::vector<HornConjunctionElement> &conjuncts, std::vector<SetFormulaHorn *> &disjuncts) {

    if(disjuncts.size() == 0) {
        Cube partial_assignment;
        return !simplify_conjunction(conjuncts, partial_assignment);
    }
    std::vector<int> clause_amounts;
    clause_amounts.reserve(disjuncts.size());
    int disj_varamount = 0;
    std::vector<int> current_clauses(disjuncts.size(), 0);
    for (SetFormulaHorn *formula : disjuncts) {
        /*
         * An empty formula is equivalent to \top
         * -> return true since everything is a subset of a union containing \top
         */
        if (formula->get_size() == 0) {
            return true;
        }
        clause_amounts.push_back(formula->get_size());
        disj_varamount = std::max(disj_varamount, formula->get_varamount());
    }

    do {
        bool unsatisfiable = false;
        Cube partial_assignment(disj_varamount, 2);
        for (size_t i = 0; i < current_clauses.size(); ++i) {
            if (unsatisfiable) {
                break;
            }
            SetFormulaHorn *formula = disjuncts[i];
            for (int left_var : formula->get_left_vars(current_clauses[i])) {
                if (partial_assignment[left_var] == 1) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[left_var] = 0;
            }
            if (unsatisfiable) {
                break;
            }
            int right_var = formula->get_right(current_clauses[i]);
            if (right_var != -1) {
                if (partial_assignment[right_var] == 0) {
                    unsatisfiable = true;
                } else {
                    partial_assignment[right_var] =1 ;
                }
            }
        }

        if (unsatisfiable) {
            continue;
        }

        if (simplify_conjunction(conjuncts, partial_assignment)) {
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
                if (i == elem.removed_implications[ri_index]) {
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
}

void SetFormulaHorn::simplify() {
    // call simplify_conjunction to get a partial assignment and implications to remove
    std::vector<HornConjunctionElement> tmpvec(1,HornConjunctionElement(this, false));
    Cube assignments(varamount, 2);
    bool satisfiable = !util->simplify_conjunction(tmpvec, assignments);

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

    if(left.size == 0) {
        // TODO: implement
    }

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

    return util->conjunction_implies_disjunction(conjunction, horn_formulas_right);
}

bool SetFormulaHorn::is_subset_with_progression(std::vector<SetFormula *> &/*left*/,
                                                std::vector<SetFormula *> &/*right*/,
                                                std::vector<SetFormula *> &/*prog*/,
                                                std::unordered_set<int> &/*actions*/) {
    // TODO:implement
    return true;
}

bool SetFormulaHorn::is_subset_with_regression(std::vector<SetFormula *> &/*left*/,
                                               std::vector<SetFormula *> &/*right*/,
                                               std::vector<SetFormula *> &/*reg*/,
                                               std::unordered_set<int> &/*actions*/) {
    // TODO:implement
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
