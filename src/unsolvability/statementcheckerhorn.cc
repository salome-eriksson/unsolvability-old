#include "statementcheckerhorn.h"
#include "global_funcs.h"

#include <cassert>
#include <iostream>
#include <stack>
#include <sstream>
#include <fstream>


Implication::Implication(std::vector<int> left, int right)
    : left(left), right(right) {

}

const std::vector<int> &Implication::get_left() const {
    return left;
}

int Implication::get_right() const {
    return right;
}

HornFormula::HornFormula(std::string input, int varamount) : varamount(varamount) {
    variable_occurences.resize(varamount);
    std::vector<std::string> clauses = determine_parameters(input,'|');
    left_size.resize(clauses.size(),-1);
    right_side.resize(clauses.size(),-2);
    std::stack<int> ft;
    std::vector<bool> ft_vec(varamount, false);

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
        int right;
        iss = std::istringstream(clauses[i].substr(delim+1));
        iss >> right;
        right_side[i] = right;
        if(left_size[i] == 0) {
            // formula is (trivially) unsatisfiable
            if(right_side[i] == -1) {
                left_size.clear;
                left_size.push_back(0);
                right_side.clear();
                right_side.push_back(-1);
                return;
            }
            ft.push(right);
        }
    }

    std::vector<std::pair<int,int>> removed_mapping;
    int clauses_size = clauses.size();

    while(!ft.empty()) {
        int var = ft.top();
        ft.pop();
        if(ft_vec[var] == true) {
            continue;
        }
        ft_vec[var] = true;

        for(const auto impl : variable_occurences[var]) {
            left_size[impl]--;
            if(left_size[impl] == 0) {
                // formula is unsatisfiable
                if(right_side[impl] == -1) {
                    left_size.clear();
                    left_size.push_back(0);
                    right_side.clear();
                    right_side.push_back(-1);
                    return;
                }
                ft.push(right_side[impl]);
                clauses_size--;
                left_size[impl] = left_size[clauses_size];
                right_side[impl] = right_side[clauses_size];
                removed_mapping.push_back(std::make_pair(impl,clauses_size));
            }
        }
        variable_occurences[var].clear();
    }

    for(size_t i = 0; i < variable_occurences.size(); ++i) {
        for(size_t j = 0; j < removed_mapping.size(); ++j) {
            bool old_found = variable_occurences[i].find(removed_mapping[i].second) !=
                    variable_occurences[i].end();
            variable_occurences[i].erase(removed_mapping[i].second);
            if(!old_found) {
                variable_occurences[i].erase(removed_mapping[i].first);
            }
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

void HornFormula::dump() const{
    /*for(int i = 0; i < implications.size(); ++i) {
        for(int j = 0; j < implications[i].get_left().size(); ++j) {
            std::cout << implications[i].get_left().at(j) << ",";
        }
        std::cout << ";"<<implications[i].get_right() << "|";
    }
    std::cout << std::endl;*/
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
        /*std::vector<std::string> clauses = determine_parameters(line.substr(delim+1),'|');
        for(int i = 0; i < clauses.size(); ++i) {
            delim = clauses[i].find(",");
            assert(delim != std::string::npos);
            std::istringstream iss(clauses[i].substr(0,delim));
            std::vector<int> left;
            int tmp;
            while(iss >> tmp) {
                left.push_back(tmp);
            }
            int right;
            iss = std::istringstream(clauses[i].substr(delim+1));
            iss >> right;
            implications.push_back(Implication(left,right));
        }*/
        stored_formulas.insert(std::make_pair(name,HornFormula(line.substr(delim+1),varamount)));
    }

    // insert true set
    stored_formulas.insert(std::make_pair("true",HornFormula(std::vector<Implication>(),varamount)));

    // insert empty set
    std::vector<Implication> empty_impl;
    empty_impl.push_back(Implication(std::vector<int>(),-1));
    stored_formulas.insert(std::make_pair("empty",HornFormula(empty_impl,varamount)));

    // insert goal
    std::vector<Implication> goal_impl;
    const Cube &goal = task->get_goal();
    for(int i = 0; i < goal.size(); ++i) {
        if(goal.at(i) == 1) {
            goal_impl.push_back(Implication(std::vector<int>(),i));
        }
    }
    stored_formulas.insert(std::make_pair("S_G",HornFormula(goal_impl, varamount)));

    // insert initial state
    std::vector<Implication> init_impl;
    const Cube &init = task->get_initial_state();
    for(int i = 0; i < init.size(); ++i) {
        if(init.at(i) == 1) {
            init_impl.push_back(Implication(std::vector<int>(),i));
        } else {
            init_impl.push_back(Implication(std::vector<int>(1,i),-1));
        }
    }
    stored_formulas.insert(std::make_pair("S_I",HornFormula(init_impl, varamount)));

    // insert action formulas
    action_formulas.reserve(task->get_number_of_actions());
    for(int actionsize = 0; actionsize < task->get_number_of_actions(); ++actionsize) {
        std::vector<Implication> actionimpl;
        const Action & action = task->get_action(actionsize);
        for(int i = 0; i < action.pre.size(); ++i) {
            actionimpl.push_back(Implication(std::vector<int>(),action.pre[i]));
        }
        for(int i = 0; i < varamount; ++i) {
            // add effect
            if(action.change[i] == 1) {
                actionimpl.push_back(Implication(std::vector<int>(),i+varamount));
            // delete effect
            } else if(action.change[i] == -1) {
                actionimpl.push_back(Implication(std::vector<int>(1,i+varamount),-1));
            // no change -> frame axiom
            } else {
                actionimpl.push_back(Implication(std::vector<int>(1,i),i+varamount));
                actionimpl.push_back(Implication(std::vector<int>(1,i+varamount),i));
            }
        }
        action_formulas.push_back(HornFormula(actionimpl,varamount*2));
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

    std::vector<std::pair<HornFormula *,bool>> elements;
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
                elements.push_back(std::make_pair(&(stored_formulas.find(item)->second),false));
                count++;
            }
        }
        assert(count == 1);
        stored_formulas.insert(std::make_pair(line, HornFormula(elements)));
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

bool StatementCheckerHorn::is_restricted_satisfiable(const HornFormulaList &formulas, const Cube &restrictions) {
    Cube solution;
    return is_restricted_satisfiable(formulas, restrictions, solution);
}

// implementation note: if restriction has the wrong size, it gets forced resized with
// don't care as fill value
bool StatementCheckerHorn::is_restricted_satisfiable(const HornFormulaList &formulas,
                                                     const Cube &restrictions, Cube &solution) {
    assert(formulas.size >= 1);
    int varamount = formulas[0].first.get_varamount();
    // total amount of implications
    int implamount = 0;
    std::stack<int> forced_true;
    // the index in which the implications for this formula start in left_count
    std::vector<int> implstart(formulas.size(),-1);

    for(size_t i = 0; i < formulas.size(); ++i) {
        implstart[i] = implamount;
        varamount = std::max(varamount, formulas[i].first.get_varamount());
        implamount += formulas[i].first.get_size();
    }

    solution.resize(varamount);
    restrictions.resize(varamount,2);
    std::fill(solution.begin(), solution.end(), false);
    std::vector<int> left_count(implamount,-1);

    for(size_t i = 0; i < formulas.size(); ++i) {
        const HornFormula &formula = formulas[i].first;
        const std::vector<int> &formula_forced_true = formula.get_forced_true();
        int offset = 0;
        if(formulas[i].second) {
            offset = varamount;
        }
        for(size_t j = 0; j < formula_forced_true.size(); ++j) {
            if(restrictions[formula_forced_true[j]+offset] == 0) {
                return false;
            }
            forced_true.push_back(formula_forced_true[j]+offset);
        }
        for(size_t j = 0; j < formula.size(); ++j) {
            left_count[implstart[i]+j] = formula.get_implication(j).get_left().size();
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
            // if the variable is primed but not the formula or vice versa we don't need to change anything
            if((var >= varamount && !formulas[findex].second) ||
               (var < varamount && formulas[findex].second)) {
                continue;
            }
            int offset = 0;
            if(formulas[findex].second) {
                offset = varamount;
            }
            HornFormula &formula = formulas[findex].first;
            std::vector<int> &var_occurences = formula.get_variable_occurence(var-offset);
            for(int i = 0; i < var_occurences.size(); ++i) {
                int impl_number = var_occurences.at(i) + implstart[findex];
                left_count[impl_number]--;
                if(left_count[impl_number] == 0) {
                    int right = formula.get_implication(impl_number).get_rght() + offset;
                    if(right == -1 || restrictions[right] == 0) {
                        return false;
                    }
                    forced_true.push(right);
                }
            }
        }
    }
}

bool StatementCheckerHorn::implies(const HornFormulaList &formulas,
                                   const HornFormula &right, bool right_primed) {
    assert(formulas.size() >= 1);
    int varamount = formulas[0].first.get_varamount();
    for(int i = 0; i < formulas.size(); ++i) {
        varamount = std::max(varamount, formulas[i].first.get_varamount());
    }
    Cube restrictions(right.get_varamount(),2);
    int offset = 0;
    if(right_primed) {
        offstet += varamount;
    }

    // If formula1 implies formula2, then formula1 \land \lnot formula 2 is unsatisfiable.
    // Since Horn formulas don't support negation but the negation is a disjunction over
    // unit clauses we can test for each disjunction separately if
    // formula1 \land disjunction is unsatisfiable.
    for(int i = 0; i < right.get_size(); ++i) {
        const Implication &impl = right.get_implication(i);
        for(int j = 0; j < impl.get_left().size(); ++j) {
            restrictions[impl.get_left().at(j)+offset] = 1;
        }
        if(impl.get_right() != -1) {
            restrictions[impl.get_right()+offset] = 0;
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
    std::vector<const HornFormula &,bool> left;
    left.push_back(std::make_pair(stored_formulas.find(set1)->second,false));
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
    subformulas().reserve(3);
    subformulas.push_back(std::make_pair(formula1, false));
    subformulas.push_back(std::make_pair(formula2, true));
    //dummy initialization
    subformulas.push_back(std::make_pair(formula1, false));

    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        subformulas[2] = std::make_pair(&action_formulas[i],false);
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
    subformulas().reserve(3);
    subformulas.push_back(std::make_pair(formula1, true));
    subformulas.push_back(std::make_pair(formula2, false));
    //dummy initialization
    subformulas.push_back(std::make_pair(formula1, false));

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
    HornFormulaList list = HornFormulaList(std::make_pair(stored_formulas.find(set)->second,false);
    if(is_restricted_satisfiable(list, state)) {
        return true;
    }
    return false;

}

bool StatementCheckerHorn::check_initial_contained(const std::string &set) {
    assert(stored_formulas.find(set) != stored_formulas.end());
    HornFormulaList list = HornFormulaList(std::make_pair(stored_formulas.find(set)->second,false);
    if(is_restricted_satisfiable(list, task->get_initial_state())) {
        return true;
    }
    return false;
}

bool StatementCheckerHorn::check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) {
    assert(stored_formulas.find(set) != stored_formulas.end());
    HornFormulaList list = HornFormulaList(std::make_pair(stored_formulas.find(set)->second,false);

    Cube x(formulas[0].first.get_varamount(), 2);
    Cube y(formulas[0].first.get_varamount(), 2);

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
