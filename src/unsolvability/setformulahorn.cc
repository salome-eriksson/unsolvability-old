#include "setformulahorn.h"

SpecialFormulasHorn::SpecialFormulasHorn(Task *task) {

}

SetFormulaHorn::SetFormulaHorn()
{

    if (special_formulas == nullptr) {
        setup_special_formulas();
    }

    simplify();
}

void SetFormulaHorn::setup_special_formulas() {

}

void SetFormulaHorn::simplify() {
    std::stack<int> ft;
    for(size_t i = 0; i < forced_true.size(); ++i) {
        ft.push(i);
    }
    std::vector<int> removed_impl;
    size_t amount_implications = left_vars.size();

    // find trivially forced true variable
    for(int i = 0; i < amount_implications; ++i) {
        if(left_vars[i].empty()) {
            // formula is (trivially) unsatisfiable
            if(right_side[i] == -1) {
                left_vars.clear();
                left_vars.push_back(std::vector<int>());
                right_side.clear();
                right_side.push_back(-1);
                variable_occurences.clear();
                variable_occurences.resize(varamount);
                forced_true.clear();
                forced_false.clear();
                return;
            }
            ft.push(right_side[i]);
            removed_impl.push_back(i);
            variable_occurences[right_side[i]].second.erase(i);
        }
    }

    std::vector<bool> ft_vec(varamount, false);
    int ft_amount = 0;

    // generalized dikstra
    while(!ft.empty()) {
        int var = ft.top();
        ft.pop();
        if(ft_vec[var]) {
            continue;
        }
        ft_vec[var] = true;
        ft_amount++;
        // all implications where var occurs on the right side are trivially true now
        for(const auto impl : variable_occurences[var].second) {
            removed_impl.push_back(impl);
            for(int i = 0; i < left_vars[impl].size(); ++i) {
                variable_occurences[left_vars[impl][i]].first.erase(impl);
            }
            left_vars[impl].clear();
        }
        variable_occurences[var].second.clear();

        for(const auto impl : variable_occurences[var].first) {
            left_vars[impl].erase(std::remove(left_vars[impl].begin(), left_vars[impl].end(), var), left_vars[impl].end());
            if(left_vars[impl].empty()) {
                // formula is unsatisfiable
                if(right_side[impl] == -1) {
                    left_vars.clear();
                    left_vars.push_back(std::vector<int>());
                    right_side.clear();
                    right_side.push_back(-1);
                    variable_occurences.clear();
                    variable_occurences.resize(varamount);
                    forced_true.clear();
                    forced_false.clear();
                    return;
                }
                ft.push(right_side[impl]);
                removed_impl.push_back(impl);
                variable_occurences[right_side[impl]].second.erase(impl);
            }
        }
        variable_occurences[var].first.clear();
    }

    std::stack<int> ff;
    int ff_amount = 0;
    std::vector<bool> ff_vec(varamount, false);
    for(size_t i = 0; i < forced_false.size(); ++i) {
        ff.push(i);
    }
    for(int i = 0; i < amount_implications; ++i) {
        if(right_side[i] == -1 && left_vars[i].size() == 1) {
            ff.push(left_vars[i][0]);
        }
    }

    while(!ff.empty()) {
        int var = ff.top();
        ff.pop();
        if(ff_vec[var]) {
            continue;
        }
        ff_vec[var] = true;
        ff_amount++;
        // all implications where var occurs on the left side are trivially true now
        for(const auto impl : variable_occurences[var].first) {
            removed_impl.push_back(impl);
            for(int i = 0; i < left_vars[impl].size(); ++i) {
                if(left_vars[impl][i] != var) {
                    variable_occurences[left_vars[impl][i]].first.erase(impl);
                }
            }
            left_vars[impl].clear();
            if(right_side[impl] != -1) {
                variable_occurences[right_side[impl]].second.erase(impl);
            }
        }
        variable_occurences[var].first.clear();

        for(const auto impl: variable_occurences[var].second) {
            right_side[impl] = -1;
            if(left_vars[impl].size() == 1) {
                ff.push(left_vars[impl][0]);
                variable_occurences[left_vars[impl][0]].first.erase(impl);
                removed_impl.push_back(impl);
                left_vars[impl].clear();
            }
        }
        variable_occurences[var].second.clear();
    }

    /*
     * TODO: update example
     * replace all empty implications with non-empty implication from the back of the vector
     * example: left_size: 2 3 0 4 0 5, removed_impl = 2 4 (must be sorted!)
     * --> new left_size 2 3 5 4 0 0, then we can resize the vector to 2 3 5 4
     */
    std::sort(removed_impl.begin(), removed_impl.end());
    int old_location = amount_implications-1;
    for(size_t i = 0; i < removed_impl.size(); ++i) {
        int new_location = removed_impl[i];
        //find the implication with the highest index number that is not going to be deleted
        while(left_vars[old_location].empty()) {
            old_location--;
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
        left_vars[new_location].swap(left_vars[old_location]);

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

    // remove empty implications
    right_side.resize(amount_implications-removed_impl.size());
    left_vars.resize(amount_implications-removed_impl.size());

    // push all forced true variables into the forced_true vector
    forced_true.clear();
    forced_true.reserve(ft_amount);
    for(size_t i = 0; i < ft_vec.size(); ++i) {
        if(ft_vec[i]) {
            forced_true.push_back(i);
        }
    }
    forced_false.clear();
    forced_false.reserve(ff_amount);
    for(size_t i = 0; i < ff_vec.size(); ++i) {
        if(ff_vec[i]) {
            forced_false.push_back(i);
        }
    }

    left_sizes.resize(left_vars.size());
    for(int i = 0; i < left_vars.size(); ++i) {
        left_sizes[i] = left_vars[i].size();
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

static bool SetFormulaHorn::is_satisfiable(const HornFormulaList &formulas, bool partial) {
    Cube restrictions, solution;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}

static bool SetFormulaHorn::is_satisfiable(const HornFormulaList &formulas, Cube &solution,
                                          bool partial) {
    Cube restrictions;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}

static bool SetFormulaHorn::is_restricted_satisfiable(const HornFormulaList &formulas,
                                                     const Cube &restrictions, bool partial) {
    Cube solution;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}


static bool SetFormulaHorn::is_restricted_satisfiable(const HornFormulaList &formulas,
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

        const HornFormula *f = formulas[i].first;
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
        const HornFormula *formula = fentry.first;
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
            const HornFormula *formula = formulas[findex].first;
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
            const HornFormula *f = formulas[findex].first;
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
                const HornFormula *formula = formulas[findex].first;
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
bool StatementCheckerHorn::implies(const HornFormulaList &formulas,const HornFormulaList &right) {
    assert(formulas.size() >= 1 && right.size() >= 1);

    Cube left_solution_partial;
    if(!is_satisfiable(formulas, left_solution_partial, true)) {
        return true;
    }

    for(size_t index = 0;  index < right.size(); ++index) {
        const HornFormula *right_formula = right.at(index).first;
        int right_varamount = right_formula->get_varamount();
        int offset = 0;
        if(right.at(index).second) {
            offset += right_varamount;
            right_varamount *= 2;
        }

        Cube restrictions = Cube(right_varamount,2);
        // first loop over all forced true
        for(size_t i = 0; i < right_formula->get_forced_true().size(); ++ i) {
            int forced_true = right_formula->get_forced_true().at(i)+offset;
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
        for(size_t i = 0; i < right_formula->get_forced_false().size(); ++i) {
            int forced_false = right_formula->get_forced_false().at(i)+offset;
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
        for(size_t i = 0; i < right_formula->get_size(); ++i) {
            const std::vector<int> &left_vars = right_formula->get_left_vars(i);
            for(size_t j = 0; j < left_vars.size(); ++j) {
                restrictions[left_vars.at(j)+offset] = 1;
            }
            if(right_formula->get_right(i) != -1) {
                // if the current implication is (a \lor ... \lor \lnot a)
                // then formula1 \land \lnot a \land .. \land a is unsatisfiable
                if(restrictions[right_formula->get_right(i)+offset] == 1) {
                    continue;
                }
                restrictions[right_formula->get_right(i)+offset] = 0;
            }
            if(is_restricted_satisfiable(formulas, restrictions)) {
                return false;
            }
            std::fill(restrictions.begin(), restrictions.end(),2);
        }
    }
    return true;
}







bool SetFormulaHorn::is_subset(SetFormula *f, bool negated, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(f);
    }
    switch (f->get_formula_type()) {
    case SetFormulaType::HORN:
        break;
    case SetFormulaType::BDD:
        break;
    case SetFormulaType::TWOCNF:
        break;
    case SetFormulaType::EXPLICIT:
        break;
    default:
        std::cerr << "X \subseteq X' is not supported for Horn formula X "
                     "and non-basic or constant formula X'" << std::endl;
        return false;
        break;
    }
}

bool SetFormulaHorn::is_subset(SetFormula *f1, SetFormula *f2) {
    if(f1->get_formula_type() == SetFormulaType::CONSTANT) {
        f1 = get_constant_formula(f1);
    }
    if(f2->get_formula_type() == SetFormulaType::CONSTANT) {
        f2 = get_constant_formula(f2);
    }
    if(f1->get_formula_type() != SetFormulaType::HORN || f2->get_formula_type() != SetFormulaType::HORN) {
        std::cerr << "X \subseteq X' \cup X'' is not supported for Horn formula X "
                     "and non-Horn formula X' or X''" << std::endl;
        return false;
    }

}

bool SetFormulaHorn::intersection_with_goal_is_subset(SetFormula *f, bool negate, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(f1);
    } else if(f->get_formula_type() != SetFormulaType::HORN) {
        std::cerr << "L \cap S_G(\Pi) \subseteq L' is not supported for Horn Formula L"
                     "and nonHorn formula L'" << std::endl;
        return false;
    }

}

bool SetFormulaHorn::progression_is_union_subset(SetFormula *f, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(f1);
    } else if(f->get_formula_type() != SetFormulaType::HORN) {
        std::cerr << "X[A] \subseteq X \cup X' is not supported for Horn Formula X"
                     "and nonHorn formula X'" << std::endl;
        return false;
    }
}

bool SetFormulaHorn::regression_is_union_subset(SetFormula *f, bool f_negated) {
    if(f->get_formula_type() == SetFormulaType::CONSTANT) {
        f = get_constant_formula(f1);
    } else if(f->get_formula_type() != SetFormulaType::HORN) {
        std::cerr << "[A]X \subseteq X \cup X' is not supported for Horn Formula X"
                     "and nonHorn formula X'" << std::endl;
        return false;
    }
}


bool SetFormulaHorn::get_formula_type() {
    return SetFormulaType::HORN;
}

SetFormulaBasic *SetFormulaHorn::get_constant_formula(SetFormulaConstant *c_formula) {
    switch(c_formula->get_constant_type()) {
    case ConstantType::EMPTY:
        return special_formulas->emptyformula;
        break;
    case ConstantType::INIT:
        return special_formulas->initformula;
        break;
    case ConstantType::GOAL:
        return special_formulas->goalformula;
        break;
    default:
        std::cerr << "Unknown Constant type: " << c << std::endl;
        return nullptr;
        break;
    }
}
