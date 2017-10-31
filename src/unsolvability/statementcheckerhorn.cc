#include "statementcheckerhorn.h"
#include "global_funcs.h"

#include <cassert>
#include <iostream>
#include <stack>
#include <sstream>
#include <fstream>
#include <algorithm>

HornFormula::HornFormula(std::vector<int> &left_size, std::vector<int> &right_side,
                         std::vector<std::unordered_set<int> > &variable_occurences, int varamount)
    : left_size(left_size), right_side(right_side),
      variable_occurences(variable_occurences), varamount(varamount) {
    simplify();
    set_left_vars();
}

HornFormula::HornFormula(std::string input, int varamount) : varamount(varamount) {
    variable_occurences.resize(varamount);
    std::vector<std::string> clauses = determine_parameters(input,'|');
    left_size.resize(clauses.size());
    right_side.resize(clauses.size());

    // parse input
    for(int i = 0; i < clauses.size(); ++i) {
        int delim = clauses[i].find(",");
        assert(delim != std::string::npos);
        std::istringstream iss(clauses[i].substr(0,delim));
        int count = 0;
        int tmp;
        while(iss >> tmp) {
            variable_occurences[tmp].insert(i);
            count++;
        }
        left_size[i] = count;
        iss = std::istringstream(clauses[i].substr(delim+1));
        iss >> right_side[i];
    }
    simplify();
    set_left_vars();
}

HornFormula::HornFormula(HornFormulaList &subformulas) {
    assert(subformulas.size() > 1);
    varamount = 0;
    int size = 0;

    for(size_t i = 0; i < subformulas.size(); ++i) {
        int local_varamount = subformulas[i].first->get_varamount();
        if(subformulas[i].second) {
            local_varamount *= 2;
        }
        varamount = std::max(varamount, local_varamount);
        size += subformulas[i].first->get_size();
    }
    left_size.reserve(size);
    right_side.reserve(size);

    variable_occurences = std::vector<std::unordered_set<int>>(varamount);
    int impl_count = 0;
    for(size_t i = 0; i < subformulas.size(); ++i) {
        const HornFormula *formula = subformulas[i].first;
        left_size.insert(left_size.end(), formula->left_size.begin(), formula->left_size.end());
        right_side.insert(right_side.end(), formula->right_side.begin(), formula->right_side.end());
        int forced_true_old_size = forced_true.size();
        forced_true.insert(forced_true.end(), formula->forced_true.begin(), formula->forced_true.end());
        int forced_false_old_size = forced_false.size();
        forced_false.insert(forced_false.end(), formula->forced_false.begin(), formula->forced_false.end());

        int offset = 0;
        if(subformulas[i].second) {
            offset += formula->get_varamount();
            for(int j = impl_count; j < left_size.size(); ++j) {
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
            for(int elem : formula->get_variable_occurence(j)) {
                variable_occurences[j+offset].insert(elem+impl_count);
            }
        }
        impl_count = left_size.size();
    }

    simplify();
    set_left_vars();
}

void HornFormula::simplify() {
    std::stack<int> ft;
    int ft_amount = 0;
    std::vector<bool> ft_vec(varamount, false);
    int ff_amount = 0;
    std::vector<bool> ff_vec(varamount, false);
    for(size_t i = 0; i < forced_true.size(); ++i) {
        if(!ft_vec[forced_true[i]]) {
            ft_vec[forced_true[i]] = true;
            ft_amount++;
        }
    }
    for(size_t i = 0; i < forced_false.size(); ++i) {
        if(!ff_vec[forced_false[i]]) {
            ff_vec[forced_false[i]] = true;
            ff_amount++;
        }
    }
    std::vector<int> removed_impl;
    int amount_implications = left_size.size();

    // find trivially forced true variable
    for(int i = 0; i < amount_implications; ++i) {
        if(left_size[i] == 0) {
            // formula is (trivially) unsatisfiable
            if(right_side[i] == -1 || ff_vec[right_side[i]]) {
                left_size.clear();
                left_size.push_back(0);
                right_side.clear();
                right_side.push_back(-1);
                variable_occurences.clear();
                variable_occurences.resize(varamount);
                return;
            }
            ft.push(right_side[i]);
            removed_impl.push_back(i);
        // implication = \lnot x --> x forced false
        } else if(left_size[i] == 1 && right_side[i] == -1) {
            int var = -1;
            for(size_t j = 0; j < variable_occurences.size(); ++j) {
                if(variable_occurences[j].erase(i) == 1) {
                    var = j;
                    break;
                }
            }
            assert(var != -1);
            ff_vec[var] = true;
            ff_amount++;
            left_size[i]--;
            removed_impl.push_back(i);
        }
    }

    // generalized dikstra
    while(!ft.empty()) {
        int var = ft.top();
        ft.pop();
        if(ft_vec[var] == true) {
            continue;
        }
        ft_vec[var] = true;
        ft_amount++;

        for(const auto impl : variable_occurences[var]) {
            left_size[impl]--;
            if(left_size[impl] == 0) {
                // formula is unsatisfiable
                if(right_side[impl] == -1 || ff_vec[right_side[impl]]) {
                    left_size.clear();
                    left_size.push_back(0);
                    right_side.clear();
                    right_side.push_back(-1);
                    variable_occurences.clear();
                    variable_occurences.resize(varamount);
                    return;
                }
                ft.push(right_side[impl]);
                removed_impl.push_back(impl);
            // implication = \lnot x --> x forced false
            } else if(left_size[impl] == 1 && right_side[impl] == -1) {
                int var = -1;
                for(size_t j = 0; j < variable_occurences.size(); ++j) {
                    if(variable_occurences[j].erase(impl) == 1) {
                        var = j;
                        break;
                    }
                }
                assert(var != -1);
                ff_vec[var] = true;
                ff_amount++;
                left_size[impl]--;
                removed_impl.push_back(impl);
            }
        }
        variable_occurences[var].clear();
    }

    /*
     * replace all empty implications with non-empty implication from the back of the vector
     * example: left_size: 2 3 0 4 0 5, removed_impl = 2 4 (must be sorted!)
     * --> new left_size 2 3 5 4 0 0, then we can resize the vector to 2 3 5 4
     */
    std::sort(removed_impl.begin(), removed_impl.end());
    int old_location = amount_implications-1;
    for(size_t i = 0; i < removed_impl.size(); ++i) {
        int new_location = removed_impl[i];
        // assert that the implication is empty and the right side is -1 or forced true
        assert(left_size[new_location] == 0);
        assert(right_side[new_location] == -1 || ft_vec[right_side[new_location]]);
        //find the implication with the highest index number that is not going to be deleted
        while(left_size[old_location] == 0) {
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
        left_size[new_location] = left_size[old_location];
        right_side[new_location] = right_side[old_location];

        // update variable_occurences
        for(size_t j = 0; j < variable_occurences.size(); ++j) {
            if(variable_occurences[j].erase(old_location) > 0) {
                variable_occurences[j].insert(new_location);
            }
        }
        old_location--;
    }

    // this loop just asserts that all empty implications are now at the end
    for(int i = 0; i < amount_implications-removed_impl.size(); ++i) {
       assert(left_size[i] > 0);
    }

    // remove empty implications
    left_size.resize(amount_implications-removed_impl.size());
    right_side.resize(amount_implications-removed_impl.size());

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
}

void HornFormula::set_left_vars() {
    left_vars.resize(left_size.size());
    for(size_t i = 0; i < varamount; ++i) {
        for(int elem: variable_occurences[i]) {
            left_vars[elem].push_back(i);
        }
    }
}

const std::unordered_set<int> &HornFormula::get_variable_occurence(int var) const {
    return variable_occurences[var];
}

int HornFormula::get_size() const {
    return left_size.size();
}

int HornFormula::get_varamount() const {
    return varamount;
}

int HornFormula::get_left(int index) const{
    return left_size[index];
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
    std::cout << std::endl;
    for(int i = 0; i < left_size.size(); ++i) {
        for(int j = 0; j < left_vars[i].size(); ++j) {
            std::cout << left_vars[i][j] << " ";
        }
        std::cout << "," << right_side[i] << "|";
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

    // insert true set (TODO: how to represent true?)
    stored_formulas.insert(std::make_pair("true",HornFormula("",varamount)));

    // insert empty set
    stored_formulas.insert(std::make_pair("empty",HornFormula(",-1|",varamount)));

    //used for building goal, initial state, and action formulas
    std::vector<int> left;
    std::vector<int> right;
    std::vector<std::unordered_set<int>> var_occ(varamount);
    // this is the maximum amount of implications the formulas need
    left.reserve(varamount*2);
    left.reserve(varamount*2);

    // insert goal
    const Cube &goal = task->get_goal();
    for(int i = 0; i < goal.size(); ++i) {
        if(goal.at(i) == 1) {
            left.push_back(0);
            right.push_back(i);
        }
    }
    stored_formulas.insert(std::make_pair("S_G",HornFormula(left, right, var_occ, varamount)));

    // insert initial state
    left.clear();
    right.clear();
    for(size_t i = 0; i < var_occ.size(); ++i) {
        var_occ[i].clear();
    }
    const Cube &init = task->get_initial_state();
    for(int i = 0; i < init.size(); ++i) {
        if(init.at(i) == 1) {
            left.push_back(0);
            right.push_back(i);
        } else {
            left.push_back(1);
            right.push_back(-1);
            var_occ[i].insert(left.size()-1);
        }
    }
    stored_formulas.insert(std::make_pair("S_I",HornFormula(left, right, var_occ, varamount)));

    // insert action formulas
    action_formulas.reserve(task->get_number_of_actions());
    for(int actionsize = 0; actionsize < task->get_number_of_actions(); ++actionsize) {
        left.clear();
        right.clear();
        var_occ = std::vector<std::unordered_set<int>>(varamount*2);
        const Action & action = task->get_action(actionsize);
        std::vector<bool> in_pre(varamount,false);
        for(int i = 0; i < action.pre.size(); ++i) {
            in_pre[action.pre[i]] = true;
            left.push_back(0);
            right.push_back(action.pre[i]);
        }
        for(int i = 0; i < varamount; ++i) {
            // add effect
            if(action.change[i] == 1) {
                left.push_back(0);
                right.push_back(i+varamount);
            // delete effect
            } else if(action.change[i] == -1) {
                left.push_back(1);
                right.push_back(-1);
                var_occ[i+varamount].insert(left.size()-1);
            // no change
            } else {
                // if var i is in pre but does not change, it must be true after the action application
                if(in_pre[i]) {
                    left.push_back(0);
                    right.push_back(i+varamount);
                // if var i is not in pre, we need to use frame axioms
                } else {
                    left.push_back(1);
                    right.push_back(i+varamount);
                    var_occ[i].insert(left.size()-1);
                    left.push_back(1);
                    right.push_back(i);
                    var_occ[i+varamount].insert(left.size()-1);
                }
            }
        }
        action_formulas.push_back(HornFormula(left, right, var_occ,varamount*2));
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

    HornFormulaList elements;
    //count asserts that the composite formula is syntactically correct
    int count = 0;
    while(line.compare("composite formulas end") != 0) {
        count = 0;
        std::stringstream ss;
        ss.str(line);
        std::string item;
        while(std::getline(ss, item, ' ')) {
            if(item.compare(KnowledgeBase::INTERSECTION) == 0) {
                assert(count >=2);
                count--;
            } else if(item.compare(KnowledgeBase::UNION) == 0) {
                std::cout << "UNION not supported for Horn composite formulas" << std::endl;
                exit_with(ExitCode::CRITICAL_ERROR);
            } else if(item.compare(KnowledgeBase::NEGATION) == 0) {
                std::cout << "UNION not supported for Horn composite formulas" << std::endl;
                exit_with(ExitCode::CRITICAL_ERROR);
            } else {
                assert(stored_formulas.find(item) != stored_formulas.end());
                elements.push_back(std::make_pair(&stored_formulas.find(item)->second,false));
                count++;
            }
        }
        assert(count == 1);
        stored_formulas.insert(std::make_pair(line, HornFormula(elements)));
        elements.clear();
        std::getline(in, line);
    }
}

bool StatementCheckerHorn::is_satisfiable(const HornFormulaList &formulas) {
    Cube restrictions, solution;
    return is_restricted_satisfiable(formulas, restrictions, solution);
}

bool StatementCheckerHorn::is_satisfiable(const HornFormulaList &formulas, Cube &solution) {
    Cube restrictions;
    return is_restricted_satisfiable(formulas, restrictions, solution);
}

bool StatementCheckerHorn::is_restricted_satisfiable(const HornFormulaList &formulas, Cube &restrictions) {
    Cube solution;
    return is_restricted_satisfiable(formulas, restrictions, solution);
}

// implementation note: if restriction has the wrong size, it gets forced resized with
// don't care as fill value
bool StatementCheckerHorn::is_restricted_satisfiable(const HornFormulaList &formulas,
                                                     Cube &restrictions, Cube &solution) {
    assert(formulas.size() >= 1);
    int varamount = formulas[0].first->get_varamount();
    // total amount of implications
    int implamount = 0;
    std::stack<int> forced_true;
    // the index for which the implications for this formula start in left_count
    std::vector<int> implstart(formulas.size());

    for(size_t i = 0; i < formulas.size(); ++i) {
        implstart[i] = implamount;
        int localvaramount = formulas[i].first->get_varamount();
        int offset = 0;
        if(formulas[i].second) {
            offset = localvaramount;
            localvaramount *= 2;
        }
        varamount = std::max(varamount, localvaramount);
        implamount += formulas[i].first->get_size();
        const std::vector<int> &forced_false = formulas[i].first->get_forced_false();
        for(size_t j = 0; j < forced_false.size(); ++j) {
            if(restrictions[forced_false[j]+offset] == 1) {
                return false;
            } else {
                restrictions[forced_false[j]+offset] = 0;
            }
        }
    }

    solution.resize(varamount);
    restrictions.resize(varamount,2);
    std::fill(solution.begin(), solution.end(), 0);
    std::vector<int> left_count(implamount,-1);

    for(size_t i = 0; i < restrictions.size(); ++i) {
        if(restrictions[i] == 1) {
            forced_true.push(i);
        }
    }

    for(size_t i = 0; i < formulas.size(); ++i) {
        const HornFormula *formula = formulas[i].first;
        const std::vector<int> &formula_forced_true = formula->get_forced_true();
        int offset = 0;
        if(formulas[i].second) {
            offset = formula->get_varamount();
        }
        for(size_t j = 0; j < formula_forced_true.size(); ++j) {
            if(restrictions[formula_forced_true.at(j)+offset] == 0) {
                return false;
            }
            forced_true.push(formula_forced_true.at(j)+offset);
        }
        for(int j = 0; j < formula->get_size(); ++j) {
            left_count[implstart[i]+j] = formula->get_left(j);
            if(left_count[implstart[i]+j] == 0) {
                int right_with_offset = formula->get_right(j) + offset;
                if(formula->get_right(j) == -1 || restrictions[right_with_offset] == 0) {
                    return false;
                }
                forced_true.push(right_with_offset);
            }
        }
    }

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
            const std::unordered_set<int> &var_occurences = formula->get_variable_occurence(var-offset);
            for(int internal_impl_number: var_occurences) {
                int impl_number = internal_impl_number + implstart[findex];
                left_count[impl_number]--;
                if(left_count[impl_number] == 0) {
                    int right_with_offset = formula->get_right(internal_impl_number) + offset;
                    if(formula->get_right(internal_impl_number) == -1 || restrictions[right_with_offset] == 0) {
                        return false;
                    }
                    forced_true.push(right_with_offset);
                }
            }
        }
    }
    return true;
}

bool StatementCheckerHorn::implies(const HornFormulaList &formulas,
                                   const HornFormula &right_formula, bool right_primed) {
    assert(formulas.size() >= 1);
    int right_varamount = right_formula.get_varamount();
    int offset = 0;
    if(right_primed) {
        offset += right_varamount;
        right_varamount *= 2;
    }
    Cube restrictions = Cube(right_varamount,2);

    /* If formula1 implies formula2, then formula1 \land \lnot formula 2 is unsatisfiable.
     * Since Horn formulas don't support negation but the negation is a disjunction over
     * unit clauses we can test for each disjunction separately if
     * formula1 \land disjunction is unsatisfiable.
     * special cases:
     *  - if the right formula is empty (= true) then it correctly returns true since we don't enter the loop
     *  - if the right formula contains an empty implication (no left side, right side = -1 --> empty)
     *    we return false only if the formulalist is satisfiable without restrictions (since we do not set any)
     */

    // first loop over all forced true
    for(size_t i = 0; i < right_formula.get_forced_true().size(); ++ i) {
        int forced_true = right_formula.get_forced_true().at(i)+offset;
        restrictions[forced_true] = 0;
        if(is_restricted_satisfiable(formulas, restrictions)) {
            return false;
        }
        restrictions[forced_true] = 2;
    }

    // now loop over implications
    for(size_t i = 0; i < right_formula.get_size(); ++i) {
        const std::vector<int> &left_vars = right_formula.get_left_vars(i);
        for(size_t j = 0; j < left_vars.size(); ++j) {
            restrictions[left_vars.at(j)+offset] = 1;
        }
        if(right_formula.get_right(i) != -1) {
            // if the current implication is (a \lor ... \lor \lnot a)
            // then formula1 \land \lnot a \land .. \land a is unsatisfiable
            if(restrictions[right_formula.get_right(i)+offset] == 1) {
                continue;
            }
            restrictions[right_formula.get_right(i)+offset] = 0;
        }
        if(is_restricted_satisfiable(formulas, restrictions)) {
            return false;
        }
        std::fill(restrictions.begin(), restrictions.end(),2);
    }
    return true;
}

bool StatementCheckerHorn::check_subset(const std::string &set1, const std::string &set2) {
    assert(stored_formulas.find(set1) != stored_formulas.end() && stored_formulas.find(set2) != stored_formulas.end());
    HornFormulaList left;
    left.push_back(std::make_pair(&stored_formulas.find(set1)->second,false));
    HornFormula &formula2 = stored_formulas.find(set2)->second;
    if(implies(left, formula2, false)) {
        return true;
    }
    return false;
}

// set2 must be the negation of a Horn formula
// Let phi_1 denote set1 and \lnot phi_2 denote set2. Then we need to show for each operator o_i
// that phi_1 \land t_o_i \implies phi_1' \lor \lnot phi_2', which is equal to
// phi_1 \land t_o_i \land phi_2' implies phi_1'
bool StatementCheckerHorn::check_progression(const std::string &set1, const std::string &set2) {
    assert(stored_formulas.find(set1) != stored_formulas.end());
    HornFormula &formula1 = stored_formulas.find(set1)->second;
    size_t not_pos = set2.find(" " + KnowledgeBase::NEGATION);
    if(not_pos == std::string::npos) {
        std::cout << set2 << " is not a negation" << std::endl;
        return false;
    }
    std::string set2_neg = set2.substr(0,set2.size()-4);
    assert(stored_formulas.find(set2_neg) != stored_formulas.end());
    HornFormula &formula2 = stored_formulas.find(set2_neg)->second;

    HornFormulaList subformulas;
    subformulas.reserve(3);
    subformulas.push_back(std::make_pair(&formula1, false));
    subformulas.push_back(std::make_pair(&formula2, true));
    //dummy initialization
    subformulas.push_back(std::make_pair(&formula1, false));

    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        subformulas[2] = std::make_pair(&action_formulas[i],false);
        //HornFormula combined = HornFormula(subformulas);
        //HornFormulaList combined_list(1,std::make_pair(&combined,false));
        if(!implies(subformulas, formula1, true)) {
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
    assert(stored_formulas.find(set1) != stored_formulas.end());
    HornFormula &formula1 = stored_formulas.find(set1)->second;
    size_t not_pos = set2.find(" " + KnowledgeBase::NEGATION);
    if(not_pos == std::string::npos) {
        std::cerr << set2 << " is not a negation" << std::endl;
        return false;
    }
    std::string set2_neg = set2.substr(0,set2.size()-4);
    assert(stored_formulas.find(set2_neg) != stored_formulas.end());
    HornFormula &formula2 = stored_formulas.find(set2_neg)->second;

    HornFormulaList subformulas;
    subformulas.reserve(3);
    subformulas.push_back(std::make_pair(&formula1, true));
    subformulas.push_back(std::make_pair(&formula2, false));
    //dummy initialization
    subformulas.push_back(std::make_pair(&formula1, false));

    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        subformulas[2] = std::make_pair(&action_formulas[i],false);
        if(!implies(subformulas, formula1, false)) {
            return false;
        }
    }
    return true;
}

bool StatementCheckerHorn::check_is_contained(Cube &state, const std::string &set) {
    assert(stored_formulas.find(set) != stored_formulas.end());
    HornFormulaList list(1,std::make_pair(&stored_formulas.find(set)->second,false));
    if(is_restricted_satisfiable(list, state)) {
        return true;
    }
    return false;

}

bool StatementCheckerHorn::check_initial_contained(const std::string &set) {
    assert(stored_formulas.find(set) != stored_formulas.end());
    HornFormulaList list(1,std::make_pair(&stored_formulas.find(set)->second,false));
    Cube init_nonconst(task->get_initial_state());
    if(is_restricted_satisfiable(list, init_nonconst)) {
        return true;
    }
    return false;
}

bool StatementCheckerHorn::check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) {
    assert(stored_formulas.find(set) != stored_formulas.end());
    HornFormulaList list(1,std::make_pair(&stored_formulas.find(set)->second,false));

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
