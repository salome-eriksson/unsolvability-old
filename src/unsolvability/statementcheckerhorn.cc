#include "statementcheckerhorn.h"

#include <cassert>
#include <iostream>
#include <stack>


Implication::Implication(std::vector<int> left, int right)
    : left(left), right(right) {

}

const std::vector<int> &Implication::get_left() const {
    return left;
}

int Implication::get_right() const {
    return right;
}

HornFormula::HornFormula(std::vector<Implication> implications, int varamount)
    : implications(implications), varamount(varamount), variable_occurences(varamount) {
    for(int i = 0; i < implications.size(); ++i) {
        const std::vector<int> &left = implications[i].get_left();
        for(int j = 0; j < left.size(); ++j) {
            variable_occurences[left[j]].push_back(i);
        }
    }
}

HornFormula::HornFormula(std::vector<std::pair<HornFormula *, bool> > &subformulas) {
    assert(subformulas.size() >= 1);
    int old_varamount = subformulas[0].first->get_varamount();
    int implications_amount = subformulas[0].first->get_size();
    bool contains_primed = false;
    for(int i = 1; i < subformulas.size(); ++i) {
        assert(old_varamount == subformulas[i].first->get_varamount());
        implications_amount += subformulas[i].first->get_size();
        if(!contains_primed && subformulas[i].second) {
            contains_primed = true;
        }
    }

    if(contains_primed) {
        varamount = old_varamount * 2;
    } else {
        varamount = old_varamount;
    }
    variable_occurences.resize(varamount);

    implications.reserve(implications_amount);
    int count = 0;
    for(int i = 0; i < subformulas.size(); ++i) {
        int primed = subformulas[i].second;
        HornFormula formula = *(subformulas[i].first);
        if(!primed) {
            implications.insert(implications.end(), formula.begin(), formula.end());
        } else {
            for(int j = 0; j < formula.get_size(); ++j) {
                std::vector<int> left(formula.get_implication(j).get_left());
                for(int k = 0; k < left.size(); ++k) {
                    left[k] += old_varamount;
                }
                implications.push_back(Implication(left, formula.get_implication(j).get_right()));
            }
        }
        // if the formula should be primed, we need to change varoccurences for the primed vars
        int offset = 0;
        if(primed) {
            offset = varamount;
        }
        for(int j = 0; j < old_varamount;++j) {
            const std::vector<int> var_occ = formula.get_variable_occurence(j);
            for(int k = 0; k < var_occ.size(); ++k) {
                // count accounts for the shift in implication numbering
                // for example if we are at the second formula and the first formula
                // had 10 implications then implication[2] in the second formula
                // is implication[12] in the combined formula
                variable_occurences[j + offset].push_back(var_occ.at(k)+count);
            }
        }
        count += formula.get_size();
    }
}

const std::vector<int> &HornFormula::get_variable_occurence(int var) const {
    return variable_occurences[var];
}

int HornFormula::get_size() const {
    return implications.size();
}

int HornFormula::get_varamount() const {
    return varamount;
}

std::vector<Implication>::const_iterator HornFormula::begin() const {
    return implications.begin();
}

std::vector<Implication>::const_iterator HornFormula::end() const {
    return implications.end();
}

const Implication &HornFormula::get_implication(int index) const {
    return implications[index];
}


StatementCheckerHorn::StatementCheckerHorn(KnowledgeBase *kb, Task *task)
    : StatementChecker(kb,task) {


}

bool StatementCheckerHorn::is_satisfiable(const HornFormula &formula) {
    Cube restrictions(formula.get_varamount(),2), solution;
    return is_satisfiable(formula, restrictions, solution);
}

bool StatementCheckerHorn::is_satisfiable(const HornFormula &formula, Cube &solution) {
    Cube restrictions(formula.get_varamount(),2);
    return is_satisfiable(formula, restrictions, solution);
}

bool StatementCheckerHorn::is_satisfiable(const HornFormula &formula, const Cube &restrictions) {
    Cube solution;
    return is_satisfiable(formula, restrictions, solution);
}

bool StatementCheckerHorn::is_satisfiable(const HornFormula &formula,
                                          const Cube &restrictions, Cube &solution) {
    assert(restrictions.size() == formula.get_varamount());
    solution.resize(formula.get_varamount());
    std::fill(solution.begin(), solution.end(), false);
    std::stack<int> forced_true;
    std::vector<int> left_count(0,formula.get_size());

    // counts for each implication how many left side variables are not forced true yet
    for(int i = 0; i < formula.get_size(); ++i) {
        const Implication &impl = formula.get_implication(i);
        left_count[i] = impl.get_left().size();
        // right is forced true since left is empty
        if(left_count[i] == 0) {
            // right is restricted to false -> unsatisfiable
            if(restrictions[impl.get_right()] == 0) {
                return false;
            }
            forced_true.push(impl.get_right());
        }
    }

    // if restriction[i] == 1 then we want var i to be true
    for(int i = 0; i < restrictions.size(); ++i) {
        if(restrictions[i] == 1) {
            forced_true.push(i);
        }
    }

    while(!forced_true.empty()) {
        int var = forced_true.top();
        forced_true.pop();
        if(solution[var] == 1) {
            continue;
        }
        solution[var] = 1;
        const std::vector<int> &var_occurences = formula.get_variable_occurence(var);
        for(int i = 0; i < var_occurences.size(); ++i) {
            int impl_number = var_occurences.at(i);
            left_count[impl_number]--;
            if(left_count[impl_number] == 0) {
                int right = formula.get_implication(impl_number).get_right();
                // no right side
                if(right == -1) {
                    return false;
                }
                // right restricted to false
                if(restrictions[right] == 0) {
                    return false;
                }
                forced_true.push(right);
            }
        }
    }
    return true;

}

bool StatementCheckerHorn::implies(const HornFormula &formula1, const HornFormula &formula2) {
    assert(formula1.get_varamount() == formula2.get_varamount());
    Cube restrictions(formula1.get_varamount(),2);

    // If formula1 implies formula2, then formula1 \land \lnot formula 2 is unsatisfiable.
    // Since Horn formulas don't support negation but the negation is a disjunction over
    // unit clauses we can test for each disjunction separately if
    // formula1 \land disjunction is unsatisfiable.
    for(int i = 0; i < formula2.get_size(); ++i) {
        const Implication &impl = formula2.get_implication(i);
        for(int j = 0; j < impl.get_left().size(); ++j) {
            restrictions[impl.get_left().at(j)] = 1;
        }
        if(impl.get_right() != -1) {
            restrictions[impl.get_right()] = 0;
        }
        if(is_satisfiable(formula1, restrictions)) {
            return false;
        }
        std::fill(restrictions.begin(), restrictions.end(),2);
    }
    return true;
}

bool StatementCheckerHorn::check_subset(const std::string &set1, const std::string &set2) {
    assert(formulas.find(set1) != formulas.end() && formulas.find(set2) != formulas.end());
    HornFormula &formula1 = formulas.find(set1)->second;
    HornFormula &formula2 = formulas.find(set2)->second;
    if(implies(formula1, formula2)) {
        return true;
    }
    return false;
}

// set2 must be the negation of a Horn formula
// Let phi_1 denote set1 and \lnot phi_2 denote set2. Then we need to show for each operator o_i
// that phi_1 \land t_o_i \implies phi_1' \lor \lnot phi_2', which is equal to
// phi_1 \land t_o_i \land phi_2' implies phi_1'
bool StatementCheckerHorn::check_progression(const std::string &set1, const std::string &set2) {
    assert(formulas.find(set1) != formulas.end());
    HornFormula &formula1 = formulas.find(set1)->second;
    size_t not_pos = set2.find(" " + KnowledgeBase::NEGATION);
    if(not_pos == std::string::npos) {
        std::cout << set2 << "is not a negation" << std::endl;
        return false;
    }
    std::string set2_neg = set2.substr(0,set2.size()-4);
    assert(formulas.find(set2) != formulas.end());
    HornFormula &formula2 = formulas.find(set2_neg)->second;

    std::vector<std::pair<HornFormula *,bool>> subformulas;
    subformulas.push_back(std::make_pair(&formula1, false));
    subformulas.push_back(std::make_pair(&formula2, true));
    HornFormula f1_and_f2_primed = HornFormula(subformulas);

    subformulas.clear();
    subformulas.push_back(std::make_pair(&formula1,true));
    HornFormula f1_primed = HornFormula(subformulas);

    subformulas.clear();
    subformulas.resize(2);
    subformulas[0] = std::make_pair(&f1_and_f2_primed,false);

    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        subformulas[1] = std::make_pair(&action_formulas[i],false);
        HornFormula left = HornFormula(subformulas);
        if(!implies(left, f1_primed)) {
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
    assert(formulas.find(set1) != formulas.end());
    HornFormula &formula1 = formulas.find(set1)->second;
    size_t not_pos = set2.find(" " + KnowledgeBase::NEGATION);
    if(not_pos == std::string::npos) {
        std::cerr << set2 << "is not a negation" << std::endl;
        return false;
    }
    std::string set2_neg = set2.substr(0,set2.size()-4);
    assert(formulas.find(set2) != formulas.end());
    HornFormula &formula2 = formulas.find(set2_neg)->second;

    std::vector<std::pair<HornFormula *,bool>> subformulas;
    subformulas.push_back(std::make_pair(&formula1, true));
    subformulas.push_back(std::make_pair(&formula2, false));
    HornFormula f1_primed_and_f2 = HornFormula(subformulas);

    subformulas.clear();
    subformulas.resize(2);
    subformulas[0] = std::make_pair(&f1_primed_and_f2,false);

    for(int i = 0; i < task->get_number_of_actions(); ++i) {
        subformulas[1] = std::make_pair(&action_formulas[i],false);
        HornFormula left = HornFormula(subformulas);
        if(!implies(left, formula1)) {
            return false;
        }
    }
    return true;
}

bool StatementCheckerHorn::check_is_contained(Cube &state, const std::string &set) {
    assert(formulas.find(set) != formulas.end());
    HornFormula &formula = formulas.find(set)->second;
    if(is_satisfiable(formula, state)) {
        return true;
    }
    return false;

}

bool StatementCheckerHorn::check_initial_contained(const std::string &set) {
    assert(formulas.find(set) != formulas.end());
    HornFormula &formula = formulas.find(set)->second;
    if(is_satisfiable(formula, task->get_initial_state())) {
        return true;
    }
    return false;
}

bool StatementCheckerHorn::check_set_subset_to_stateset(const std::string &set, const StateSet &stateset) {
    assert(formulas.find(set) != formulas.end());
    HornFormula &formula = formulas.find(set)->second;

    Cube x(formula.get_varamount(), 2);
    Cube y(formula.get_varamount(), 2);

    Cube *old_solution = &x;
    Cube *new_solution = &y;

    if(!is_satisfiable(formula,*old_solution)) {
        return true;
    }
    std::vector<int> mapping;
    for(int i = 0; i < old_solution->size(); ++i) {
        if(old_solution->at(i) == 0) {
            mapping.push_back(i);
        }
    }
    int n = mapping.size();
    std::vector<bool> marking(n,0);
    bool new_solution_found = true;
    // guaranteed to stop after at most |stateset| iterations
    // (since stateset can only contain at most |stateset| distinct assignments)
    while(new_solution_found) {
        for(int i = n-1; i >= 0; --i) {
            if(!marking[i]) {
                  old_solution->at(mapping[i]) = 1-old_solution->at(mapping[i]);
                  if(is_satisfiable(formula, *old_solution, *new_solution)) {
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
                      Cube *tmp = old_solution;
                      old_solution = new_solution;
                      new_solution = tmp;
                      break;
                  }
            }
            old_solution->at(mapping[i]) = 2;
        }
        new_solution_found = false;
    }
    return true;
}





