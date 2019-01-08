#include "setformulahorn.h"

#include <fstream>
#include <deque>
#include <algorithm>
#include <cassert>
#include <iostream>

#include "global_funcs.h"

HornConjunctionElement::HornConjunctionElement (const SetFormulaHorn *formula)
    : formula(formula), removed_implications(formula->get_size(), false) {
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

    hornactions.reserve(task->get_number_of_actions());
    for(int a = 0; a < task->get_number_of_actions(); ++a) {
        const Action & action = task->get_action(a);
        clauses.clear();

        for(int i = 0; i < action.pre.size(); ++i) {
            clauses.push_back(std::make_pair(std::vector<int>(),action.pre[i]));
        }
        SetFormulaHorn *pre = new SetFormulaHorn(clauses, varamount);
        clauses.clear();

        std::vector<int> shift_vars;
        for(int i = 0; i < varamount; ++i) {
            // add effect
            if(action.change[i] == 1) {
                clauses.push_back(std::make_pair(std::vector<int>(),i+varamount));
                shift_vars.push_back(i);
            // delete effect
            } else if(action.change[i] == -1) {
                clauses.push_back(std::make_pair(std::vector<int>(1,i+varamount),-1));
                shift_vars.push_back(i);
            }
        }
        SetFormulaHorn *eff = new SetFormulaHorn(clauses, varamount);
        hornactions.push_back(HornAction(pre,eff,shift_vars));
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

bool HornUtil::simplify_conjunction(std::vector<HornConjunctionElement> &conjuncts, Cube &partial_assignment) {
    int total_varamount = 0;
    int amount_implications = 0;
    std::vector<int> implstart(conjuncts.size(),-1);

    std::deque<std::pair<int,bool>> unit_clauses;
    std::vector<int> leftcount;
    std::vector<int> rightvars;

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
        total_varamount = std::max(total_varamount, local_varamount);
        implstart[i] = amount_implications;
        amount_implications += conjunct.formula->get_size();
        partial_assignment.resize(total_varamount,2);
        leftcount.insert(leftcount.end(), conjunct.formula->get_left_sizes().begin(),
                         conjunct.formula->get_left_sizes().end());
        rightvars.insert(rightvars.end(), conjunct.formula->get_right_sides().begin(),
                        conjunct.formula->get_right_sides().end());

        for(int var : conjunct.formula->get_forced_false()) {
            if (partial_assignment[var] == 1) {
                return false;
            }
            unit_clauses.push_back(std::make_pair(var, false));
            partial_assignment[var] = 0;
        }
        for(int var : conjunct.formula->get_forced_true()) {
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
                if (var >= conjunct.formula->get_varamount()) {
                    continue;
                }
                const std::forward_list<int> &left_occurences = conjunct.formula->get_variable_occurence_left(var);
                const std::forward_list<int> &right_occurences = conjunct.formula->get_variable_occurence_right(var);
                for (int left_occ : left_occurences) {
                    int global_impl = implstart[i]+left_occ;
                    leftcount[global_impl]--;
                    // deleted last negative literal --> must have positive literal
                    if (leftcount[global_impl] == 0) {
                        int newvar = conjunct.formula->get_right(left_occ);
                        assert(newvar != -1);
                        if (partial_assignment[newvar] == 0) {
                            return false;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, true));
                        partial_assignment[newvar] = 1;
                        conjunct.removed_implications[left_occ] = true;
                    // deleted second last negative literal and no positive literal
                    } else if (leftcount[global_impl] == 1
                               && rightvars[global_impl] == -1) {
                        // find corresponding variable in left
                        int newvar = -1;
                        for (int leftvar : conjunct.formula->get_left_vars(left_occ)) {
                            if (partial_assignment[leftvar] != 1) {
                                newvar = leftvar;
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
                if (var >= conjunct.formula->get_varamount()) {
                    continue;
                }
                const std::forward_list<int> &left_occurences = conjunct.formula->get_variable_occurence_left(var);
                const std::forward_list<int> &right_occurences = conjunct.formula->get_variable_occurence_right(var);
                for (int left_occ : left_occurences) {
                    conjunct.removed_implications[left_occ] = true;
                }
                for(int right_occ: right_occurences) {
                    rightvars[implstart[i]+right_occ] = -1;
                    if(leftcount[implstart[i]+right_occ] == 1) {
                        // find corresponding variable in left
                        int newvar = -1;
                        for (int leftvar : conjunct.formula->get_left_vars(right_occ)) {
                            if (partial_assignment[leftvar] != 1) {
                                newvar = leftvar;
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
    std::vector<HornConjunctionElement> vec(1,HornConjunctionElement(formula));
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


bool HornUtil::conjunction_implies_disjunction(std::vector<SetFormulaHorn *> &conjuncts,
                                               std::vector<SetFormulaHorn *> &disjuncts) {

    if (conjuncts.size() == 0) {
        conjuncts.push_back(trueformula);
    }
    std::vector<int> clause_amounts;
    std::vector<int> current_clauses;
    int disj_varamount = 0;
    // iterator for removing while iterating
    std::vector<SetFormulaHorn *>::iterator disjunct_it = disjuncts.begin();
    while (disjunct_it != disjuncts.end()) {
        SetFormulaHorn *formula = *disjunct_it;
        /*
         * An empty formula is equivalent to \top
         * -> return true since everything is a subset of a union containing \top
         */
        if (formula->get_size() + formula->get_forced_false().size()
                + formula->get_forced_true().size() == 0) {
            return true;
        }
        // formula not satisfiable -> ignore it
        Cube dummy;
        if (!is_restricted_satisfiable(formula, dummy)) {
            disjunct_it = disjuncts.erase(disjunct_it);
            continue;
        }
        clause_amounts.push_back(formula->get_forced_true().size()
                                 +formula->get_forced_false().size()
                                 +formula->get_size());
        current_clauses.push_back(0);
        int local_varamount = formula->get_varamount();
        disj_varamount = std::max(disj_varamount, local_varamount);
        disjunct_it++;
    }

    SetFormulaHorn conjunction_dummy(task);
    const SetFormulaHorn *conjunction = conjuncts[0];
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
            const SetFormulaHorn *formula = disjuncts[i];
            int clausenum = current_clauses[i];
            if (clausenum < formula->get_forced_true().size()) {
                int forced_true = formula->get_forced_true().at(clausenum);
                if (partial_assignment[forced_true] == 1
                        || conjunction_pa[forced_true] == 1) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[forced_true] = 0;
            }

            clausenum -= formula->get_forced_true().size();
            if (clausenum >= 0 && clausenum < formula->get_forced_false().size()) {
                int forced_false = formula->get_forced_false().at(clausenum);
                if (partial_assignment[forced_false] == 0
                        || conjunction_pa[forced_false] == 0) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[forced_false] = 1;
            }

            clausenum -= formula->get_forced_false().size();
            if (clausenum >= 0) {
                for (int left_var : formula->get_left_vars(clausenum)) {
                    if (partial_assignment[left_var] == 0
                            || conjunction_pa[left_var] == 0) {
                        unsatisfiable = true;
                        break;
                    }
                    partial_assignment[left_var] = 1;
                }
                int right_var = formula->get_right(clausenum);
                if (right_var != -1) {
                    if (partial_assignment[right_var] == 1
                            || conjunction_pa[right_var] == 1) {
                        unsatisfiable = true;
                        break;
                    }
                    partial_assignment[right_var] = 0;
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
    //simplify();
    varorder.reserve(varamount);
    for (size_t var = 0; var < varamount; ++var) {
        varorder.push_back(var);
    }
}

SetFormulaHorn::SetFormulaHorn(std::vector<SetFormulaHorn *> &formulas) {
    std::vector<HornConjunctionElement> elements;
    elements.reserve(formulas.size());
    for (SetFormulaHorn *f : formulas) {
        elements.emplace_back(HornConjunctionElement(f));
    }
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
    std::vector<HornConjunctionElement> tmpvec(1,HornConjunctionElement(this));
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

void SetFormulaHorn::shift(std::vector<int> &vars) {
    /*variable_occurences.resize(varamount*2);
    for (int var : vars) {
        for (int impl : variable_occurences[var].first) {
            for (size_t i = 0; i < left_vars[impl].size(); ++i) {
                if (left_vars[impl][i] == var) {
                    left_vars[impl][i] += varamount;
                    break;
                }
            }
        }
        for (int impl : variable_occurences[var].second) {
            right_side[impl] += varamount;
        }
        variable_occurences[var+varamount] = std::move(variable_occurences[var]);
        variable_occurences[var] =
                std::make_pair(std::forward_list<int>(), std::forward_list<int>());
    }
    varamount *=2;*/
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
    return util->conjunction_implies_disjunction(horn_formulas_left, horn_formulas_right);
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

    if(util->hornactions.size() == 0) {
        util->build_actionformulas();
    }

    SetFormulaHorn *prog_singular;
    SetFormulaHorn prog_dummy(util->task);
    if (horn_formulas_prog.size() > 1) {
        prog_dummy = SetFormulaHorn(horn_formulas_prog);
        prog_singular = &prog_dummy;
    } else {
        prog_singular = horn_formulas_prog[0];
    }
    SetFormulaHorn *left_singular = nullptr;
    SetFormulaHorn left_dummy(util->task);
    if (!horn_formulas_left.empty()) {
        if (horn_formulas_left.size() > 1) {
            left_dummy = SetFormulaHorn(horn_formulas_left);
            left_singular = &left_dummy;
        } else {
            left_singular = horn_formulas_left[0];
        }
    }

    std::vector<SetFormulaHorn *> vec1,vec2;
    vec1.push_back(prog_singular);
    if (left_singular) {
        vec2.push_back(left_singular);
    }
    for (int a : actions) {
        vec1.push_back(util->hornactions[a].pre);
        SetFormulaHorn prog_pre(vec1);
        prog_pre.shift(util->hornactions[a].shift_vars);
        vec2.push_back(&prog_pre);
        vec2.push_back(util->hornactions[a].eff);

        if (!util->conjunction_implies_disjunction(vec2, horn_formulas_right)) {
            return false;
        }
        vec2.pop_back();
        vec2.pop_back();
        vec1.pop_back();
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

    if(util->hornactions.size() == 0) {
        util->build_actionformulas();
    }

    SetFormulaHorn *reg_singular;
    SetFormulaHorn reg_dummy(util->task);
    if (horn_formulas_reg.size() > 0) {
        reg_dummy = SetFormulaHorn(horn_formulas_reg);
        reg_singular = &reg_dummy;
    } else {
        reg_singular = horn_formulas_reg[0];
    }
    SetFormulaHorn *left_singular = nullptr;
    SetFormulaHorn left_dummy(util->task);
    if (!horn_formulas_left.empty()) {
        if (horn_formulas_left.size() > 1) {
            left_dummy = SetFormulaHorn(horn_formulas_left);
            left_singular = &left_dummy;
        } else {
            left_singular = horn_formulas_left[0];
        }
    }

    std::vector<SetFormulaHorn *> vec1,vec2;
    vec1.push_back(reg_singular);
    if (left_singular) {
        vec2.push_back(left_singular);
    }
    for (int a : actions) {
        vec1.push_back(util->hornactions[a].eff);
        SetFormulaHorn reg_eff(vec1);
        reg_eff.shift(util->hornactions[a].shift_vars);
        vec2.push_back(&reg_eff);
        vec2.push_back(util->hornactions[a].pre);

        if (!util->conjunction_implies_disjunction(vec2, horn_formulas_right)) {
            return false;
        }
        vec2.pop_back();
        vec2.pop_back();
        vec1.pop_back();
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
    std::vector<SetFormulaHorn *>left,right;
    left.push_back(&implicant_horn);
    right.push_back(this);
    return util->conjunction_implies_disjunction(left, right);
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
