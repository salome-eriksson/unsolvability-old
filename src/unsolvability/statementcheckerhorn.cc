#include "statementcheckerhorn.h"
#include "global_funcs.h"

#include <cassert>
#include <iostream>
#include <stack>
#include <sstream>
#include <fstream>
#include <algorithm>

HornFormula::HornFormula(const std::vector<std::pair<std::vector<int>, int> > &clauses, int varamount)
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
    }
    simplify();
}

HornFormula::HornFormula(std::string input, int varamount) : variable_occurences(varamount), varamount(varamount) {
    std::vector<std::string> clauses = determine_parameters(input,'|');
    right_side.resize(clauses.size());
    left_vars.resize(clauses.size());

    // parse input
    for(int i = 0; i < clauses.size(); ++i) {
        int delim = clauses[i].find(",");
        assert(delim != std::string::npos);
        std::istringstream iss(clauses[i].substr(0,delim));
        int tmp;
        while(iss >> tmp) {
            variable_occurences[tmp].first.insert(i);
            left_vars[i].push_back(tmp);
        }
        std::istringstream iss2(clauses[i].substr(delim+1));
        iss2 >> right_side[i];
        if(right_side[i] != -1) {
            variable_occurences[right_side[i]].second.insert(i);
        }
    }
    simplify();
}

HornFormula::HornFormula(const HornFormulaList &subformulas) {
    assert(subformulas.size() > 1);
    varamount = 0;
    int size = 0;

    for(size_t i = 0; i < subformulas.size(); ++i) {
        int local_varamount = subformulas.at(i).first->get_varamount();
        if(subformulas.at(i).second) {
            local_varamount *= 2;
        }
        varamount = std::max(varamount, local_varamount);
        size += subformulas.at(i).first->get_size();
    }
    right_side.reserve(size);

    variable_occurences.resize(varamount);
    int impl_count = 0;
    for(size_t i = 0; i < subformulas.size(); ++i) {
        const HornFormula *formula = subformulas.at(i).first;
        left_vars.insert(left_vars.end(), formula->left_vars.begin(), formula->left_vars.end());
        right_side.insert(right_side.end(), formula->right_side.begin(), formula->right_side.end());
        int forced_true_old_size = forced_true.size();
        forced_true.insert(forced_true.end(), formula->forced_true.begin(), formula->forced_true.end());
        int forced_false_old_size = forced_false.size();
        forced_false.insert(forced_false.end(), formula->forced_false.begin(), formula->forced_false.end());

        int offset = 0;
        if(subformulas.at(i).second) {
            offset += formula->get_varamount();
            for(int j = impl_count; j < left_vars.size(); ++j) {
                for(int k = 0; i < left_vars[j].size(); ++k) {
                    left_vars[j][k] += offset;
                }
                right_side[j] += offset;
            }
            for(int j = forced_true_old_size; j < forced_true.size(); ++j) {
                forced_true[j] += offset;
            }
            for(int j = forced_false_old_size; j< forced_false.size(); ++j) {
                forced_false[j] += offset;
            }
        }

        for(size_t j = 0; j < formula->get_varamount(); ++j) {
            for(int elem : formula->get_variable_occurence_left(j)) {
                variable_occurences[j+offset].first.insert(elem+impl_count);
            }
            for(int elem : formula->get_variable_occurence_right(j)) {
                variable_occurences[j+offset].second.insert(elem+impl_count);
            }
        }
        impl_count = left_vars.size();
    }
    simplify();
}

void HornFormula::simplify() {
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

const std::unordered_set<int> &HornFormula::get_variable_occurence_left(int var) const {
    return variable_occurences[var].first;
}

const std::unordered_set<int> &HornFormula::get_variable_occurence_right(int var) const {
    return variable_occurences[var].second;
}

int HornFormula::get_size() const {
    return left_vars.size();
}

int HornFormula::get_varamount() const {
    return varamount;
}

int HornFormula::get_left(int index) const{
    return left_vars[index].size();
}

const std::vector<int> &HornFormula::get_left_sizes() const {
    return left_sizes;
}

int HornFormula::get_right(int index) const {
    return right_side[index];
}

const std::vector<int> &HornFormula::get_forced_true() const {
    return forced_true;
}

const std::vector<int> &HornFormula::get_forced_false() const {
    return forced_false;
}

const std::vector<int> &HornFormula::get_left_vars(int index) const {
    return left_vars[index];
}

void HornFormula::dump() const{
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


StatementCheckerHorn::StatementCheckerHorn(KnowledgeBase *kb, Task *task, std::ifstream &in)
    : StatementChecker(kb,task) {

    std::string line;
    // first line contains file location for horn formulas
    std::getline(in,line);
    std::ifstream formulas_file;
    formulas_file.open(line);
    if(!formulas_file.is_open()) {
        std::cout << "could not find formula file \"" << line << "\" for Horn Statementchecker" << std::endl;
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }
    int varamount = task->get_number_of_facts();
    while(std::getline(formulas_file,line)) {
        int delim = line.find(":");
        assert(delim != std::string::npos);
        std::string name = line.substr(0,delim);
        stored_formulas.insert(std::make_pair(name,HornFormula(line.substr(delim+1),varamount)));
    }

    // insert true set
    stored_formulas.insert(std::make_pair(special_set_string(SpecialSet::TRUESET), HornFormula("",varamount)));

    // insert empty set
    stored_formulas.insert(std::make_pair(special_set_string(SpecialSet::EMPTYSET),HornFormula(",-1|",varamount)));

    //used for building goal, initial state, and action formulas
    std::vector<std::pair<std::vector<int>,int>> clauses;
    // this is the maximum amount of clauses the formulas need
    clauses.reserve(varamount*2);

    // insert goal
    const Cube &goal = task->get_goal();
    for(int i = 0; i < goal.size(); ++i) {
        if(goal.at(i) == 1) {
            clauses.push_back(std::make_pair(std::vector<int>(),i));
        }
    }
    stored_formulas.insert(std::make_pair(special_set_string(SpecialSet::GOALSET),HornFormula(clauses, varamount)));

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
    stored_formulas.insert(std::make_pair(special_set_string(SpecialSet::INITSET),HornFormula(clauses, varamount)));

    // insert action formulas
    action_formulas.reserve(task->get_number_of_actions());
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
        action_formulas.push_back(HornFormula(clauses,varamount*2));
    }


    std::getline(in, line);
    // read in composite formulas
    if(line.compare("composite formulas begin") == 0) {
        read_in_composite_formulas(in);
        std::getline(in, line);
    }
    // second last line contains location of statement file
    statementfile = line;

    // last line declares end of Statement block
    std::getline(in, line);
    assert(line.compare("Statements:Horn end") == 0);
}

void StatementCheckerHorn::read_in_composite_formulas(std::ifstream &in) {
    std::string line;
    std::getline(in, line);
    while(line.compare("composite formulas end") != 0) {
        HornFormulaList list = get_formulalist(line);
        stored_formulas.insert(std::make_pair(line, HornFormula(list)));
        std::getline(in, line);
    }
}

HornFormulaList StatementCheckerHorn::get_formulalist(const std::string &description) {
    HornFormulaList list;
    //count asserts that the formula is syntactically correct
    int count = 0;
    std::stringstream ss;
    ss.str(description);
    std::string item;
    while(std::getline(ss, item, ' ')) {
        if(item.compare(KnowledgeBase::INTERSECTION) == 0) {
            assert(count >=2);
            count--;
        } else if(item.compare(KnowledgeBase::UNION) == 0) {
            std::cout << "UNION not supported for Horn composite formulas" << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        } else if(item.compare(KnowledgeBase::NEGATION) == 0) {
            std::cout << "NEGATION not supported for Horn composite formulas" << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        } else {
            auto found = stored_formulas.find(description);
            found = stored_formulas.find(item);
            assert(found != stored_formulas.end());
            list.push_back(std::make_pair(&found->second,false));
            count++;
        }
    }
    assert(count == 1);
    return list;
}

bool StatementCheckerHorn::is_satisfiable(const HornFormulaList &formulas, bool partial) {
    Cube restrictions, solution;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}

bool StatementCheckerHorn::is_satisfiable(const HornFormulaList &formulas, Cube &solution,
                                          bool partial) {
    Cube restrictions;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}

bool StatementCheckerHorn::is_restricted_satisfiable(const HornFormulaList &formulas,
                                                     const Cube &restrictions, bool partial) {
    Cube solution;
    return is_restricted_satisfiable(formulas, restrictions, solution, partial);
}


bool StatementCheckerHorn::is_restricted_satisfiable(const HornFormulaList &formulas,
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

bool StatementCheckerHorn::check_subset(const std::string &set1, const std::string &set2) {
    HornFormulaList left = get_formulalist(set1);
    HornFormulaList right = get_formulalist(set2);
    if(implies(left, right)) {
        return true;
    }
    return false;
}

// set2 must be the negation of a Horn formula
// Let phi_1 denote set1 and \lnot phi_2 denote set2. Then we need to show for each operator o_i
// that phi_1 \land t_o_i \implies phi_1' \lor \lnot phi_2', which is equal to
// phi_1 \land t_o_i \land phi_2' implies phi_1'
bool StatementCheckerHorn::check_progression(const std::string &set1, const std::string &set2) {
    HornFormulaList subformulas = get_formulalist(set1);
    size_t not_pos = set2.find(" " + KnowledgeBase::NEGATION);
    if(not_pos == std::string::npos) {
        std::cout << set2 << " is not a negation" << std::endl;
        return false;
    }
    std::string set2_neg = set2.substr(0,set2.size()-4);
    HornFormulaList formula2_list = get_formulalist(set2_neg);

    HornFormulaList formula1_primed(subformulas);
    for(size_t i = 0; i < formula1_primed.size(); ++i) {
        formula1_primed[i].second = true;
    }

    for(size_t i = 0; i < formula2_list.size(); ++i) {
        subformulas.push_back(std::make_pair(formula2_list[i].first, true));
    }
    //dummy initialization
    subformulas.push_back(std::make_pair(nullptr, false));

    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        subformulas[subformulas.size()-1] = std::make_pair(&action_formulas[i],false);
        if(!implies(subformulas, formula1_primed)) {
            return false;
        }
    }
    return true;
}

// set2 must be the negation of a Horn formula
// Let phi_1 denote set1 and \lnot phi_2 denote set2. Then we need to show for each operator o_i
// that phi_1' \land t_o_i implies phi_1 \lor \lnot phi_2, which is equal to
// phi_1' \land t_o_i \land phi_2 implies phi_1
bool StatementCheckerHorn::check_regression(const std::string &set1, const std::string &set2) {
    HornFormulaList formula1_list = get_formulalist(set1);
    size_t not_pos = set2.find(" " + KnowledgeBase::NEGATION);
    if(not_pos == std::string::npos) {
        std::cout << set2 << " is not a negation" << std::endl;
        return false;
    }
    std::string set2_neg = set2.substr(0,set2.size()-4);
    HornFormulaList subformulas = get_formulalist(set2_neg);

    for(size_t i = 0; i < formula1_list.size(); ++i) {
        subformulas.push_back(std::make_pair(formula1_list[i].first, true));
    }
    //dummy initialization
    subformulas.push_back(std::make_pair(nullptr, false));

    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        subformulas[subformulas.size()-1] = std::make_pair(&action_formulas[i],false);
        if(!implies(subformulas, formula1_list)) {
            return false;
        }
    }
    return true;
}

bool StatementCheckerHorn::check_is_contained(Cube &state, const std::string &set) {
    if(is_restricted_satisfiable(get_formulalist(set), state)) {
        return true;
    }
    return false;

}

bool StatementCheckerHorn::check_initial_contained(const std::string &set) {
    if(is_restricted_satisfiable(get_formulalist(set), task->get_initial_state())) {
        return true;
    }
    return false;
}

bool StatementCheckerHorn::check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) {
    HornFormulaList list = get_formulalist(set);

    Cube x(list[0].first->get_varamount(), 2);
    Cube y(list[0].first->get_varamount(), 2);

    Cube *old_solution = &x;
    Cube *new_solution = &y;

    if(!is_restricted_satisfiable(list,*old_solution,*new_solution)) {
        return true;
    } else if(!stateset.contains(*new_solution)) {
        std::cout << "stateset does not contain the following cube:";
        for(int i = 0; i < new_solution->size(); ++i) {
            std::cout << new_solution->at(i) << " ";
        } std::cout << std::endl;
        return false;
    }
    std::vector<int> mapping;
    for(int i = 0; i < new_solution->size(); ++i) {
        if(new_solution->at(i) == 0) {
            mapping.push_back(i);
        }
    }
    int n = mapping.size();
    std::vector<bool> marking(n,0);
    bool new_solution_found = true;
    // guaranteed to stop after at most |stateset| iterations
    // (since stateset can only contain at most |stateset| distinct assignments)
    while(new_solution_found) {
        Cube *tmp = old_solution;
        old_solution = new_solution;
        new_solution = tmp;
        for(int i = n-1; i >= 0; --i) {
            if(!marking[i]) {
                  old_solution->at(mapping[i]) = 1-old_solution->at(mapping[i]);
                  if(is_restricted_satisfiable(list, *old_solution, *new_solution)) {
                      marking[i] = true;
                      for(int j = i; j < n; ++j) {
                          marking[j] = false;
                      }
                      if(!stateset.contains(*new_solution)) {
                          std::cout << "stateset does not contain the following cube:";
                          for(int i = 0; i < new_solution->size(); ++i) {
                              std::cout << new_solution->at(i) << " ";
                          } std::cout << std::endl;
                          return false;
                      }
                      break;
                  }
            }
            old_solution->at(mapping[i]) = 2;
        }
        new_solution_found = false;
    }
    return true;
}
