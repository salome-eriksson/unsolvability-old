#include "setformulaexplicit.h"
#include "setformulahorn.h"
#include "setformulabdd.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

#include "global_funcs.h"


ExplicitUtil::ExplicitUtil(Task *task)
    : task(task) {

    std::vector<int> varorder(1,0);
    std::unordered_set<std::vector<bool>> entries;
    emptyformula = SetFormulaExplicit(std::move(varorder), std::move(entries));
    varorder = std::vector<int> {0};
    entries = std::unordered_set<std::vector<bool>> {{true},{false}};
    trueformula = SetFormulaExplicit(std::move(varorder), std::move(entries));
    varorder = std::vector<int>();
    int var = 0;
    for ( int val : task->get_goal()) {
        if (val == 1) {
            varorder.push_back(var);
        }
        var++;
    }
    entries = std::unordered_set<std::vector<bool>> {std::vector<bool>(varorder.size(), true)};
    goalformula = SetFormulaExplicit(std::move(varorder),std::move(entries));
    varorder = std::vector<int>(task->get_number_of_facts(),-1);
    for (size_t i = 0; i < varorder.size(); ++i) {
        varorder[i] = i;
    }
    entries = std::unordered_set<std::vector<bool>>();
    std::vector<bool> entry(varorder.size());
    var = 0;
    for ( int val : task->get_initial_state()) {
        if (val == 1) {
            entry[var] = true;
        }
    }
    entries.insert(std::move(entry));
    initformula = SetFormulaExplicit(std::move(varorder),std::move(entries));


    hex.reserve(16);
    for(int i = 0; i < 16; ++i) {
        std::vector<bool> tmp = { (bool)((i >> 3)%2), (bool)((i >> 2)%2),
                                 (bool)((i >> 1)%2), (bool)(i%2)};
        hex.push_back(tmp);
    }

}

bool ExplicitUtil::get_explicit_vector(std::vector<SetFormula *> &formulas,
                                       std::vector<SetFormulaExplicit *> &explicit_formulas) {
    assert(explicit_formulas.empty());
    explicit_formulas.reserve(formulas.size());
    for(size_t i = 0; i < formulas.size(); ++i) {
        if (formulas[i]->get_formula_type() == SetFormulaType::CONSTANT) {
            SetFormulaConstant *c_formula = static_cast<SetFormulaConstant *>(formulas[i]);
            // see if we can use get_constant_formula
            switch (c_formula->get_constant_type()) {
            case ConstantType::EMPTY:
                explicit_formulas.push_back(&emptyformula);
                break;
            case ConstantType::GOAL:
                explicit_formulas.push_back(&goalformula);
                break;
            case ConstantType::INIT:
                explicit_formulas.push_back(&initformula);
                break;
            default:
                std::cerr << "Unknown constant type " << std::endl;
                exit_with(ExitCode::CRITICAL_ERROR);
                break;
            }
        } else if(formulas[i]->get_formula_type() == SetFormulaType::EXPLICIT) {
            SetFormulaExplicit *h_formula = static_cast<SetFormulaExplicit *>(formulas[i]);
            explicit_formulas.push_back(h_formula);
        } else {
            std::cerr << "Error: SetFormula of type other than Explicit not allowed here."
                      << std::endl;
            return false;
        }
    }
    return true;
}

std::unique_ptr<ExplicitUtil> SetFormulaExplicit::util;

SetFormulaExplicit::SetFormulaExplicit() {

}

SetFormulaExplicit::SetFormulaExplicit(std::vector<int> &&vars,
                                       std::unordered_set<std::vector<bool>> &&entries)
    : vars(vars), models(entries) {
    for (std::vector<bool> entry : this->models) {
        assert(vars.size() == entry.size());
    }

}

SetFormulaExplicit::SetFormulaExplicit(std::vector<const SetFormulaExplicit &> &conjuncts) {

}

SetFormulaExplicit::SetFormulaExplicit(std::ifstream &input, Task *task) {
    if(!util) {
        util = std::unique_ptr<ExplicitUtil>(new ExplicitUtil(task));
    }

    int varamount;
    input >> varamount;
    vars.reserve(varamount);
    for(int i = 0; i < varamount ; ++i) {
        int var;
        input >> var;
        vars.push_back(var);
    }

    std::string s;
    input >> s;

    if(s.compare(":") != 0) {
        std::cerr << "Error when parsing Explicit Formula: varamount not correct." << std::endl;
        exit_with(ExitCode::PARSING_ERROR);
    }

    // TODO: would a reserve make sense? but then we need to know the number of models

    input >> s;
    while(s.compare(";") != 0) {
        std::vector<bool> entry(varamount);
        int pos = 0;
        const std::vector<bool> *vec;
        for (int i = 0; i < (varamount/4); ++i) {
            if (s.at(i) < 'a') {
                vec = &(util->hex.at(s.at(i)-'0'));
            } else {
                vec = &(util->hex.at(s.at(i)-'a'+10));
            }
            for (int i = 0; i < 4; ++i) {
                entry[pos++] = vec->at(i);
            }
        }
        if (s.at(varamount/4) < 'a') {
            vec = &(util->hex.at(s.at(varamount/4)-'0'));
        } else {
            vec = &(util->hex.at(s.at(varamount/4)-'a'+10));
        }
        for (int i = 0; i < (varamount%4); ++i) {
            entry[pos++] = vec->at(i);
        }
        models.insert(std::move(entry));
        input >> s;
    }
}

inline std::vector<bool> get_transformed_model(GlobalModel &global_model,
                                               std::vector<GlobalModelVarOcc> var_occurences) {
    std::vector<bool> f_model(var_occurences.size(), false);
    for (size_t i = 0; i < var_occurences.size(); ++i) {
        GlobalModelVarOcc &var_occ = var_occurences[i];
        f_model[i] = global_model.at(var_occ.first)->at(var_occ.second);
    }
    return f_model;
}

bool SetFormulaExplicit::is_subset(std::vector<SetFormula *> &left,
                                   std::vector<SetFormula *> &right) {

    std::vector<SetFormulaExplicit *> left_explicit;
    std::vector<SetFormulaExplicit *> right_explicit;
    bool valid_formulas = util->get_explicit_vector(left, left_explicit)
            && util->get_explicit_vector(right, right_explicit);
    if(!valid_formulas) {
        return false;
    }

    right_explicit.erase(std::remove(right_explicit.begin(), right_explicit.end(),
                                     &(util->emptyformula)), right_explicit.end());

    // the union formulas must all talk about the same variables
    if(!right_explicit.empty()) {
        std::vector<int> &varorder = right_explicit[0]->vars;
        for(size_t i = 1; i < right_explicit.size(); ++i) {
            if(!std::is_permutation(varorder.begin(), varorder.end(),
                                    right_explicit[i]->vars.begin())) {
                std::cerr << "Union of explicit sets contains different variables." << std::endl;
                return false;
            }
        }
    }


    // left empty -> right side must be valid
    if(left_explicit.empty()) {
        // TODO: implement
        return true;
    }


    // left contains elements
    SetFormulaExplicit *reference_formula = left_explicit[0];
    std::vector<SetFormulaExplicit *> same_varorder_formulas;
    std::vector<OtherFormula> other_formulas;

    for (size_t i = 1; i < left_explicit.size(); ++i) {
        if (left_explicit[i]->vars == left_explicit[0]->vars) {
            same_varorder_formulas.push_back(left_explicit[i]);
        } else {
            other_formulas.push_back(OtherFormula(left_explicit[i]));
        }
    }

    // all formulas in conjunction have same varorder -> no transformations needed
    if(other_formulas.empty()) {
        for (const std::vector<bool> &model : reference_formula->models) {
            bool contained = true;
            for ( SetFormulaExplicit *formula : same_varorder_formulas) {
                if(!formula->contains(model)) {
                    contained = false;
                    break;
                }
            }
            if (contained) {
                // TODO: right side must contain the model
            }
        }
        return true;
    }

    // other_formulas contains elements
    GlobalModel global_model(other_formulas.size()+1, nullptr);
    std::vector<ModelExtensions> model_extensions(other_formulas.size());

    // gather for each formula where in the "global model" the corresponding vars are
    std::unordered_map <int,GlobalModelVarOcc> var_occurence_map;
    for (size_t i = 0; i < reference_formula->vars.size(); ++i)  {
        var_occurence_map.insert(std::make_pair(reference_formula->vars[i],
                                                std::make_pair(0,i)));
    }
    int index = 0;
    for (OtherFormula &other_formula : other_formulas) {
        int newvar_amount = 0;
        for (size_t i = 0; i < other_formula.formula->vars.size(); ++i) {
            int var = other_formula.formula->vars[i];
            auto pos = var_occurence_map.find(var);
            if (pos == var_occurence_map.end()) {
                GlobalModelVarOcc occ(index,newvar_amount++);
                pos = var_occurence_map.insert({var,occ}).first;
                other_formula.newvars_pos.push_back(i);
            }
            other_formula.var_occurences.push_back(pos->second);
        }
        model_extensions[index] = ModelExtensions(1,std::vector<bool>(other_formula.newvars_pos.size()));
        index++;
    }

    // loop over each model of the reference formula
    for (const std::vector<bool> &model : reference_formula->models) {

        bool contained = true;
        // For same varorder formulas no transformation is needed.
        for ( SetFormulaExplicit *formula : same_varorder_formulas) {
            if (!formula->contains(model)) {
                contained = false;
                break;
            }
        }
        // model not contained in another formula of the conjunction -> nothing to do
        if (!contained) {
            continue;
        }

        /*
         * other_formulas contain vars not occuring in reference. We need to get all
         * models of the conjunction.
         * For example:
         *  - current model vars = {0,1,3}, current model = {t,f,f}
         *  - other formula vars = {0,2,3,4}
         * --> The conjunction contains current model combined with
         *     *all* models of other formula which fit {t,*,f,*}
         */
        global_model[0] = &model;
        std::vector<int> pos(other_formulas.size(), -1);
        int f_index = 0;
        bool done = false;

        /*
         * We expect f_index to index the next other_formula we need to check
         * (and get model_extensions from).
         * If f_index = other_formula.size(), we need to check the disjunctions.
         * Furthermore, we expect all pos[i] with i < f_index to point to valid positions
         * in addon, and global_model to point to those positions.
         */
        while(!done) {
            if (f_index == other_formulas.size()) {
                // TODO: right side must contain the model
                f_index--;
            } else {
                OtherFormula &f = other_formulas[f_index];
                std::vector<bool> f_model = get_transformed_model(global_model, f.var_occurences);
                model_extensions[f_index] = f.formula->get_missing_var_values(
                            f_model, f.newvars_pos);
                pos[f_index] = -1;
            }
            while(pos[f_index] == model_extensions[f_index].size()-1) {
                f_index--;
                if(f_index == -1) {
                    return true;
                }
            }
            pos[f_index]++;
            global_model[f_index+1] = &model_extensions[f_index][pos[f_index]];
            f_index++;
        }
    }

    std::cerr << "Checking explicit subset: this point should never be reached!";
    exit_with(ExitCode::CRITICAL_ERROR);
}

bool SetFormulaExplicit::is_subset_with_progression(std::vector<SetFormula *> &/*left*/,
                                                    std::vector<SetFormula *> &/*right*/,
                                                    std::vector<SetFormula *> &/*prog*/,
                                                    std::unordered_set<int> &/*actions*/) {
    // TODO:implement
    return true;
}

bool SetFormulaExplicit::is_subset_with_regression(std::vector<SetFormula *> &/*left*/,
                                                   std::vector<SetFormula *> &/*right*/,
                                                   std::vector<SetFormula *> &/*reg*/,
                                                   std::unordered_set<int> &/*actions*/) {
    // TODO:implement
    return true;
}

bool SetFormulaExplicit::is_subset_of(SetFormula */*superset*/, bool /*left_positive*/, bool right_/*positive*/) {
    // TODO:implement
    return true;
}


SetFormulaType SetFormulaExplicit::get_formula_type() {
    return SetFormulaType::EXPLICIT;
}

SetFormulaBasic *SetFormulaExplicit::get_constant_formula(SetFormulaConstant *c_formula) {
    switch(c_formula->get_constant_type()) {
    case ConstantType::EMPTY:
        return &(util->emptyformula);
        break;
    case ConstantType::INIT:
        return &(util->initformula);
        break;
    case ConstantType::GOAL:
        return &(util->goalformula);
        break;
    default:
        std::cerr << "Unknown Constant type: " << std::endl;
        return nullptr;
        break;
    }
}

bool SetFormulaExplicit::contains(const std::vector<bool> &model) const {
    return (models.find(model) != models.end());
}

std::vector<std::vector<bool>> SetFormulaExplicit::get_missing_var_values(
        std::vector<bool> &model, const std::vector<int> &missing_vars_pos) const {
    std::vector<std::vector<bool>> ret;
    for (int count = 0; count < (1 << missing_vars_pos.size()); ++count) {
        std::vector<bool> entry(missing_vars_pos.size());
        for (size_t i = 0; i < missing_vars_pos.size(); ++i) {
            model[missing_vars_pos[i]] = ((count >> i) % 2 == 1);
        }
        if (contains(model)) {
            ret.emplace_back(entry);
        }
    }
    return ret;
}

