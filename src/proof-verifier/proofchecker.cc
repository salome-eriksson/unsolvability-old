#include "proofchecker.h"

#include <cassert>
#include <stack>
#include <fstream>
#include <math.h>
#include <limits>

#include "global_funcs.h"
#include "setformulaconstant.h"
#include "setformulabdd.h"
#include "setformulaexplicit.h"
#include "setformulahorn.h"

// TODO: should all error messages here be printed in cerr?


ProofChecker::ProofChecker(std::string &task_file)
    : task(task_file), unsolvability_proven(false) {
    expression_types = std::map<std::string,ExpressionType> {
        { "b", ExpressionType::BDD },
        { "h", ExpressionType::HORN },
        { "d", ExpressionType::DUALHORN },
        { "t", ExpressionType::TWOCNF },
        { "e", ExpressionType::EXPLICIT },
        { "c", ExpressionType::CONSTANT },
        { "n", ExpressionType::NEGATION },
        { "i", ExpressionType::INTERSECTION },
        { "u", ExpressionType::UNION },
        { "p", ExpressionType::PROGRESSION },
        { "r", ExpressionType::REGRESSION }
    };

    // TODO: understand what's going on
    using namespace std::placeholders;
    dead_knowledge_functions = {
        { "ed", std::bind(&ProofChecker::check_rule_ed, this, _1, _2, _3) },
        /*{ "ud", std::bind(&ProofChecker::check_rule_ud, this, _1, _2, _3) },
        { "sd", std::bind(&ProofChecker::check_rule_sd, this, _1, _2, _3) },
        { "pg", std::bind(&ProofChecker::check_rule_pg, this, _1, _2, _3) },
        { "pi", std::bind(&ProofChecker::check_rule_pi, this, _1, _2, _3) },
        { "rg", std::bind(&ProofChecker::check_rule_rg, this, _1, _2, _3) },
        { "ri", std::bind(&ProofChecker::check_rule_ri, this, _1, _2, _3) }*/
    };

    subset_knowledge_functions = {
        { "ura", std::bind(&ProofChecker::check_rule_ur<ActionSet>, this, _1, _2, _3, _4) },
        { "urs", std::bind(&ProofChecker::check_rule_ur<StateSet>, this, _1, _2, _3, _4) },
        /*{ "ula", std::bind(&ProofChecker::check_rule_ul<ActionSet>, this, _1, _2, _3, _4) },
        { "uls", std::bind(&ProofChecker::check_rule_ul<StateSet>, this, _1, _2, _3, _4) },
        { "ira", std::bind(&ProofChecker::check_rule_ir<ActionSet>, this, _1, _2, _3, _4) },
        { "irs", std::bind(&ProofChecker::check_rule_ir<StateSet>, this, _1, _2, _3, _4) },
        { "ila", std::bind(&ProofChecker::check_rule_il<ActionSet>, this, _1, _2, _3, _4) },
        { "ils", std::bind(&ProofChecker::check_rule_il<StateSet>, this, _1, _2, _3, _4) },
        { "dia", std::bind(&ProofChecker::check_rule_di<ActionSet>, this, _1, _2, _3, _4) },
        { "dis", std::bind(&ProofChecker::check_rule_di<StateSet>, this, _1, _2, _3, _4) },
        { "sua", std::bind(&ProofChecker::check_rule_su<ActionSet>, this, _1, _2, _3, _4) },
        { "sus", std::bind(&ProofChecker::check_rule_su<StateSet>, this, _1, _2, _3, _4) },
        { "sia", std::bind(&ProofChecker::check_rule_si<ActionSet>, this, _1, _2, _3, _4) },
        { "sis", std::bind(&ProofChecker::check_rule_si<StateSet>, this, _1, _2, _3, _4) },
        { "sta", std::bind(&ProofChecker::check_rule_st<ActionSet>, this, _1, _2, _3, _4) },
        { "sts", std::bind(&ProofChecker::check_rule_st<StateSet>, this, _1, _2, _3, _4) },*/

        { "at", std::bind(&ProofChecker::check_rule_at, this, _1, _2, _3, _4) },
        /*{ "au", std::bind(&ProofChecker::check_rule_au, this, _1, _2, _3, _4) },
        { "pt", std::bind(&ProofChecker::check_rule_pt, this, _1, _2, _3, _4) },
        { "pu", std::bind(&ProofChecker::check_rule_pu, this, _1, _2, _3, _4) },
        { "pr", std::bind(&ProofChecker::check_rule_pr, this, _1, _2, _3, _4) },
        { "rp", std::bind(&ProofChecker::check_rule_rp, this, _1, _2, _3, _4) },*/

        { "b1", std::bind(&ProofChecker::check_statement_B1, this, _1, _2, _3, _4)},
        /*{ "b2", std::bind(&ProofChecker::check_statement_B2, this, _1, _2, _3, _4)},
        { "b3", std::bind(&ProofChecker::check_statement_B3, this, _1, _2, _3, _4)},
        { "b4", std::bind(&ProofChecker::check_statement_B4, this, _1, _2, _3, _4)},
        { "b5", std::bind(&ProofChecker::check_statement_B5, this, _1, _2, _3, _4)},*/
    };

    // TODO: do we want to initialize this here?
    manager = Cudd(task.get_number_of_facts()*2);
    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();
    std::cout << "Amount of Actions: " << task.get_number_of_actions() << std::endl;
}

template<>
ActionSet *ProofChecker::get_set_expression<ActionSet>(int set_id) {
    return actionsets[set_id].get();
}

template<>
StateSet *ProofChecker::get_set_expression<StateSet>(int set_id) {
    return formulas[set_id].get();
}

void ProofChecker::add_knowledge(std::unique_ptr<Knowledge> entry, int id) {
    assert(id >= kbentries.size());
    if(id > kbentries.size()) {
        kbentries.resize(id);
    }
    kbentries.push_back(std::move(entry));
}

StateSetVariable *ProofChecker::gather_sets_intersection(StateSet *f,
                                            std::vector<StateSetVariable *> &positive,
                                            std::vector<StateSetVariable *> &negative) {
    StateSetVariable *ret = nullptr;
    switch(f->get_formula_type()) {
    case SetFormulaType::CONSTANT:
    case SetFormulaType::BDD:
    case SetFormulaType::HORN:
    case SetFormulaType::TWOCNF:
    case SetFormulaType::EXPLICIT: {
        StateSetVariable *f_var = static_cast<StateSetVariable *>(f);
        positive.push_back(f_var);
        ret = f_var;
        break;
    }
    case SetFormulaType::NEGATION:
        f = formulas[dynamic_cast<StateSetNegation *>(f)->get_child_id()].get();
        ret = gather_sets_intersection(f, negative, positive);
        break;
    case SetFormulaType::INTERSECTION: {
        StateSetIntersection *fi = dynamic_cast<StateSetIntersection *>(f);
        ret = gather_sets_intersection(formulas[fi->get_left_id()].get(),
                positive, negative);
        if(ret) {
            StateSetVariable *ret2 = gather_sets_intersection(formulas[fi->get_right_id()].get(),
                positive, negative);
            if(ret->get_formula_type() == SetFormulaType::CONSTANT) {
                ret = ret2;
            }
        }
        break;
    }
    default:
        break;
    }
    return ret;
}

StateSetVariable *ProofChecker::gather_sets_union(StateSet *f,
                                     std::vector<StateSetVariable *> &positive,
                                     std::vector<StateSetVariable *> &negative) {
    StateSetVariable *ret = nullptr;
    switch(f->get_formula_type()) {
    case SetFormulaType::CONSTANT:
    case SetFormulaType::BDD:
    case SetFormulaType::HORN:
    case SetFormulaType::TWOCNF:
    case SetFormulaType::EXPLICIT: {
        StateSetVariable *f_var = static_cast<StateSetVariable *>(f);
        positive.push_back(f_var);
        ret = f_var;
        break;
    }
    case SetFormulaType::NEGATION:
        f = formulas[dynamic_cast<StateSetNegation *>(f)->get_child_id()].get();
        ret = gather_sets_union(f, negative, positive);
        break;
    case SetFormulaType::UNION: {
        StateSetUnion *fi = dynamic_cast<StateSetUnion *>(f);
        ret = gather_sets_union(formulas[fi->get_left_id()].get(),
                positive, negative);
        if(ret) {
            StateSetVariable *ret2 = gather_sets_union(formulas[fi->get_right_id()].get(),
                positive, negative);
            if(ret->get_formula_type() == SetFormulaType::CONSTANT) {
                ret = ret2;
            }
        }
        break;
    }
    default:
        break;
    }
    return ret;
}

StateSetVariable *ProofChecker::update_reference_and_check_consistency(StateSetVariable *reference_formula, StateSetVariable *tmp, std::string stmt) {
    // if the current reference formula is constant, update to tmp
    if (reference_formula->get_formula_type() == SetFormulaType::CONSTANT) {
        reference_formula = tmp;
    }
    // if the types differ and neither is constant, throw an error
    // (reference_formula can only be constant if tmp is also constant)
    if (reference_formula->get_formula_type() != tmp->get_formula_type()
            && tmp->get_formula_type() != SetFormulaType::CONSTANT) {
        std::string msg = "Error when checking statement " + stmt
                + ": set expressions invovle different types.";
        throw std::runtime_error(msg);
    }
    return reference_formula;
}

void ProofChecker::add_state_set(std::string &line) {
    std::stringstream ssline(line);
    int expression_index;
    ssline >> expression_index;
    std::string word;
    // read in expression type
    ssline >> word;
    // the type is defined by a single character
    if (word.length() != 1) {
        std::cerr << "unknown expression type " << word << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    std::unique_ptr<StateSet> expression;

    ExpressionType type = ExpressionType::NONE;
    if (expression_types.find(word) != expression_types.end()) {
        type = expression_types.at(word);
    }

    switch(type) {
    case ExpressionType::BDD:
        expression = std::unique_ptr<StateSet>(new SetFormulaBDD(ssline, task));
        break;
    case ExpressionType::HORN:
        expression = std::unique_ptr<StateSet>(new SetFormulaHorn(ssline, task));
        break;
    case ExpressionType::DUALHORN:
        std::cerr << "not implemented yet" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
        break;
    case ExpressionType::TWOCNF:
        std::cerr << "not implemented yet" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
        break;
    case ExpressionType::EXPLICIT:
        expression = std::unique_ptr<StateSet>(new SetFormulaExplicit(ssline, task));
        break;
    case ExpressionType::CONSTANT:
        expression = std::unique_ptr<StateSet>(new SetFormulaConstant(ssline, task));
        break;
    case ExpressionType::NEGATION: {
        int subformulaindex;
        ssline >> subformulaindex;
        expression = std::unique_ptr<StateSet>(new StateSetNegation(subformulaindex));
        break;
    }
    case ExpressionType::INTERSECTION: {
        int left, right;
        ssline >> left;
        ssline >> right;
        expression = std::unique_ptr<StateSet>(new StateSetIntersection(left, right));
        break;
    }
    case ExpressionType::UNION: {
        int left, right;
        ssline >> left;
        ssline >> right;
        expression = std::unique_ptr<StateSet>(new StateSetUnion(left, right));
        break;
    }
    case ExpressionType::PROGRESSION: {
        int subformulaindex, actionsetindex;
        ssline >> subformulaindex;
        ssline >> actionsetindex;
        expression = std::unique_ptr<StateSet>(new StateSetProgression(subformulaindex, actionsetindex));
        break;
    }
    case ExpressionType::REGRESSION: {
        int subformulaindex, actionsetindex;
        ssline >> subformulaindex;
        ssline >> actionsetindex;
        expression = std::unique_ptr<StateSet>(new StateSetRegression(subformulaindex, actionsetindex));
        break;
    }
    default:
        std::cerr << "unknown expression type " << word << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    if (expression_index >= formulas.size()) {
        formulas.resize(expression_index+1);
    }
    assert(!formulas[expression_index]);
    formulas[expression_index] = std::move(expression);
}

// TODO: fix action sets
void ProofChecker::add_action_set(std::string &line) {
    std::stringstream ssline(line);
    int action_index;
    std::unique_ptr<ActionSet> action_set;
    ssline >> action_index;
    std::string type;
    // read in action type
    ssline >> type;

    if(type.compare("b") == 0) { // basic enumeration of actions
        int amount;
        // the first number denotes the amount of actions being enumerated
        std::unordered_set<int> actions;
        ssline >> amount;
        int a;
        for(int i = 0; i < amount; ++i) {
            ssline >> a;
            actions.insert(a);
        }
        action_set = std::unique_ptr<ActionSet>(new ActionSetBasic(actions));

    } else if(type.compare("u") == 0) { // union of action sets
        int left, right;
        ssline >> left;
        ssline >> right;
        action_set = std::unique_ptr<ActionSet>(new ActionSetUnion(actionsets[left].get(), actionsets[right].get()));

    } else if(type.compare("a") == 0) { // constant (denoting the set of all actions)
        action_set = std::unique_ptr<ActionSet>(new ActionSetConstantAll(task));

    } else {
        std::cerr << "unknown actionset type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    if (action_index >= actionsets.size()) {
        actionsets.resize(action_index+1);
    }
    assert(!actionsets[action_index]);
    actionsets[action_index] = std::move(action_set);
}

// TODO: unify error messages

void ProofChecker::verify_knowledge(std::string &line) {
    int knowledge_id;
    std::stringstream ssline(line);
    ssline >> knowledge_id;
    bool knowledge_is_correct = false;

    std::string word;
    // read in knowledge type
    ssline >> word;

    if(word == "s") { // subset knowledge
        int left_id, right_id, tmp;
        std::vector<int> premises;
        // reserve max amount of premises (currently 2)
        premises.reserve(2);

        ssline >> left_id;
        ssline >> right_id;
        // read in with which basic statement or derivation rule this knowledge should be checked
        ssline >> word;
        // read in premises
        while (ssline >> tmp) {
            premises.push_back(tmp);
        }

        if (subset_knowledge_functions.find(word) == subset_knowledge_functions.end()) {
            std::cerr << "unknown justification for subset knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
        knowledge_is_correct = subset_knowledge_functions[word](knowledge_id, left_id, right_id, premises);

    } else if(word == "d") { // dead knowledge
        int dead_set_id, tmp;
        std::vector<int> premises;
        // reserve max amount of premises (currently 3)
        premises.reserve(2);

        ssline >> dead_set_id;
        // read in with which derivation rule this knowledge should be checked
        ssline >> word;
        // read in premises
        while (ssline >> tmp) {
            premises.push_back(tmp);
        }

        if (dead_knowledge_functions.find(word) == dead_knowledge_functions.end()) {
            std::cerr << "unknown justification for dead set knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
        knowledge_is_correct = dead_knowledge_functions[word](knowledge_id, dead_set_id, premises);

    } else if(word == "u") { // unsolvability knowledge
        int premise;

        // read in with which derivation rule unsolvability should be proven
        ssline >> word;
        ssline >> premise;

        if (word.compare("ci")) {
            knowledge_is_correct = check_rule_ci(knowledge_id, premise);
        } else if (word.compare("cg")) {
            knowledge_is_correct = check_rule_cg(knowledge_id, premise);
        } else {
            std::cerr << "unknown justification for unsolvability knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }

    } else {
        std::cerr << "unknown knowledge type " << word << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    // TODO: necessary?
    if(!knowledge_is_correct) {
        std::cerr << "check for knowledge #" << knowledge_id << " NOT successful!" << std::endl;
    }
}


/*
 * RULES ABOUT DEADNESS
 */

// Emptyset Dead: set=emptyset is dead
bool ProofChecker::check_rule_ed(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.empty());

    SetFormulaConstant *f = dynamic_cast<SetFormulaConstant *>(formulas[stateset_id].get());
    if ((!f) || (f->get_constant_type() != ConstantType::EMPTY)) {
        std::cerr << "Error when applying rule ED: set expression #" << stateset_id
                  << " is not the constant empty set." << std::endl;
        return false;
    }
    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}

#if 0
// Union Dead: given (1) S is dead and (2) S' is dead, set=S \cup S' is dead
bool ProofChecker::check_rule_ud(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] != nullptr &&
           kbentries[premise_ids[1]] != nullptr);

    // set represents S \cup S'
    StateSetUnion *f = dynamic_cast<StateSetUnion *>(formulas[stateset_id].get());
    if (!f) {
        std::cerr << "Error when applying rule UD to conclude knowledge #" << conclusion_id
                  << ": set expression #" << stateset_id << "is not a union." << std::endl;
        return false;
    }
    int lefti = f->get_left_id();
    int righti = f->get_right_id();

    // check if premise_ids[0] says that S is dead
    if ((kbentries[premise_ids[0]].get()->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[premise_ids[0]].get()->get_first() != lefti)) {
        std::cerr << "Error when applying rule UD: Knowledge #" << premise_ids[0]
                  << "does not state that set expression #" << lefti
                  << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[1] says that S' is dead
    if ((kbentries[premise_ids[1]].get()->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[premise_ids[1]].get()->get_first() != righti)) {
        std::cerr << "Error when applying rule UD: Knowledge #" << premise_ids[1]
                  << "does not state that set expression #" << righti
                  << " is dead." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Subset Dead: given (1) S \subseteq S' and (2) S' is dead, set=S is dead
bool ProofChecker::check_rule_sd(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] != nullptr &&
           kbentries[premise_ids[1]] != nullptr);

    // check if premise_ids[0] says that S is a subset of S' (S' can be anything)
    if ((kbentries[premise_ids[0]]->get_kbentry_type() != KBType::SUBSET) ||
       (kbentries[premise_ids[0]]->get_first() != stateset_id)) {
        std::cerr << "Error when applying rule SD: knowledge #" << premise_ids[0]
                  << " does not state that set expression #" << stateset_id
                  << " is a subset of another set." << std::endl;
        return false;
    }

    int xi = kbentries[premise_ids[0]]->get_second();

    // check if premise_ids[1] says that S' is dead
    if ((kbentries[premise_ids[1]]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise_ids[1]]->get_first() != xi)) {
        std::cerr << "Error when applying rule SD: knowledge #" << premise_ids[0]
                  << " states that set expression #" << stateset_id
                  << " is a subset of set expression #" << xi << ", but knowledge #" << premise_ids[1]
                  << " does not state that " << xi << " is dead." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}

// Progression Goal: given (1) S[A] \subseteq S \cup S', (2) S' is dead and (3) S \cap S_G^\Pi is dead,
// then set=S is dead
bool ProofChecker::check_rule_pg(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           kbentries[premise_ids[0]] != nullptr &&
           kbentries[premise_ids[1]] != nullptr &&
           kbentries[premise_ids[2]] != nullptr);

    // check if premise_ids[0] says that S[A] \subseteq S \cup S'
    if (kbentries[premise_ids[0]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[0]
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is S[A]
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(formulas[kbentries[premise_ids[0]]->get_first()].get());
    if ((!s_prog) || (s_prog->get_stateset_id() != stateset_id)) {
        std::cerr << "Error when applying rule PG: the left side of subset knowledge #" << premise_ids[0]
                  << " is not the progression of set expression #" << stateset_id << "." << std::endl;
        return false;
    }
    if(!actionsets[s_prog->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule PG: "
                     "the progression does not speak about all actions" << std::endl;
        return false;
    }
    // check if the right side of premise_ids[0] is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(formulas[kbentries[premise_ids[0]]->get_second()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule PG: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << stateset_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int spi = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    if ((kbentries[premise_ids[1]]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise_ids[1]]->get_first() != spi)) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that S \cap S_G^\Pi is dead
    if (kbentries[premise_ids[2]]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[2]
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    StateSetIntersection *s_and_goal =
            dynamic_cast<StateSetIntersection *>(formulas[kbentries[premise_ids[2]]->get_first()].get());
    // check if left side of s_and_goal is S
    if ((!s_and_goal) || (s_and_goal->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule PG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with set expression #" << stateset_id
                  << " on the left side." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_and_goal->get_right_id()].get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule PG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Progression Initial: given (1) S[A] \subseteq S \cup S', (2) S' is dead and (3) {I} \subseteq S_not,
// then set=S_not is dead
bool ProofChecker::check_rule_pi(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           kbentries[premise_ids[0]] != nullptr &&
           kbentries[premise_ids[1]] != nullptr &&
           kbentries[premise_ids[2]] != nullptr);

    // check if set corresponds to s_not
    StateSetNegation *s_not = dynamic_cast<StateSetNegation *>(formulas[stateset_id].get());
    if(!s_not) {
        std::cerr << "Error when applying rule PI: set expression #" << stateset_id
                  << " is not a negation." << std::endl;
        return false;
    }
    int si = s_not->get_child_id();

    // check if premise_ids[0] says that S[A] \subseteq S \cup S'
    if (kbentries[premise_ids[0]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[0]
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is S[A]
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(formulas[kbentries[premise_ids[0]]->get_first()].get());
    if ((!s_prog) || (s_prog->get_stateset_id() != si)) {
        std::cerr << "Error when applying rule PI: the left side of subset knowledge #" << premise_ids[0]
                  << " is not the progression of set expression #" << si << "." << std::endl;
        return false;
    }
    if(!actionsets[s_prog->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule PI: "
                     "the progression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise_ids[0] is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(formulas[kbentries[premise_ids[0]]->get_second()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != si)) {
        std::cerr << "Error when applying rule PI: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << si
                  << " and another set expression." << std::endl;
        return false;
    }

    int spi = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    if ((kbentries[premise_ids[1]]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise_ids[1]]->get_first() != spi)) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that {I} \subseteq S
    if (kbentries[premise_ids[2]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[2]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of premise_ids[2] is {I}
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[premise_ids[2]]->get_first()].get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule PI: the left side of subset knowledge #" << premise_ids[2]
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of pemise3 is S
    if(kbentries[premise_ids[2]]->get_second() != si) {
        std::cerr << "Error when applying rule PI: the right side of subset knowledge #" << premise_ids[2]
                  << " is not set expression #" << si << "." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Regression Goal: given (1)[A]S \subseteq S \cup S', (2) S' is dead and (3) S_not \cap S_G^\Pi is dead,
// then set=S_not is dead
bool ProofChecker::check_rule_rg(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           kbentries[premise_ids[0]] != nullptr &&
           kbentries[premise_ids[1]] != nullptr &&
           kbentries[premise_ids[2]] != nullptr);

    // check if set corresponds to s_not
    StateSetNegation *s_not = dynamic_cast<StateSetNegation *>(formulas[stateset_id].get());
    if(!s_not) {
        std::cerr << "Error when applying rule RG: set expression #" << stateset_id
                  << " is not a negation." << std::endl;
        return false;
    }
    int si = s_not->get_child_id();

    // check if premise_ids[0] says that [A]S \subseteq S \cup S'
    if(kbentries[premise_ids[0]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[0]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is [A]S
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(formulas[kbentries[premise_ids[0]]->get_first()].get());
    if ((!s_reg) || (s_reg->get_stateset_id() != si)) {
        std::cerr << "Error when applying rule RG: the left side of subset knowledge #" << premise_ids[0]
                  << " is not the regression of set expression #" << si << "." << std::endl;
        return false;
    }
    if(!actionsets[s_reg->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule RG: "
                     "the regression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise_ids[0] is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(formulas[kbentries[premise_ids[0]]->get_second()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != si)) {
        std::cerr << "Error when applying rule RG: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << si
                  << " and another set expression." << std::endl;
        return false;
    }

    int spi = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    if ((kbentries[premise_ids[1]]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise_ids[1]]->get_first() != spi)) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that S_not \cap S_G(\Pi) is dead
    if (kbentries[premise_ids[2]]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[2]
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    StateSetIntersection *s_not_and_goal =
            dynamic_cast<StateSetIntersection *>(formulas[kbentries[premise_ids[2]]->get_first()].get());
    // check if left side of s_not_and goal is S_not
    if ((!s_not_and_goal) || (s_not_and_goal->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule RG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with set expression #" << stateset_id
                  << " on the left side." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_not_and_goal->get_right_id()].get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule RG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Regression Initial: (1) [A]S \subseteq S \cup S', (2) S' is dead and (3) {I} \subseteq S_not,
// then set=S is dead
bool ProofChecker::check_rule_ri(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           kbentries[premise_ids[0]] != nullptr &&
           kbentries[premise_ids[1]] != nullptr &&
           kbentries[premise_ids[2]] != nullptr);

    // check if premise_ids[0] says that [A]S \subseteq S \cup S'
    if(kbentries[premise_ids[0]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[0]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is [A]S
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(formulas[kbentries[premise_ids[0]]->get_first()].get());
    if ((!s_reg) || (s_reg->get_stateset_id() != stateset_id)) {
        std::cerr << "Error when applying rule RI: the left side of subset knowledge #" << premise_ids[0]
                  << " is not the regression of set expression #" << stateset_id << "." << std::endl;
        return false;
    }
    if(!actionsets[s_reg->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule RI: "
                     "the regression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise_ids[0] is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(formulas[kbentries[premise_ids[0]]->get_second()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule RI: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << stateset_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int spi = s_cup_sp->get_right_id();

    // check if k2 says that S' is dead
    if ((kbentries[premise_ids[1]]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise_ids[1]]->get_first() != spi)) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that {I} \subseteq S_not
    if (kbentries[premise_ids[2]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[2]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of premise_ids[2] is {I}
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[premise_ids[2]]->get_first()].get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule RI: the left side of subset knowledge #" << premise_ids[2]
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of premise_ids[2] is S_not
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(formulas[kbentries[premise_ids[2]]->get_second()].get());
    if((!s_not) || s_not->get_child_id() != stateset_id) {
        std::cerr << "Error when applying rule RI: the right side of subset knowledge #" << premise_ids[2]
                  << " is not the negation of set expression #" << stateset_id << "." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}
#endif

/*
 * CONCLUSION RULES
 */

// Conclusion Initial: given (1) {I} is dead, the task is unsolvable
bool ProofChecker::check_rule_ci(int conclusion_id, int premise_id) {
    assert(kbentries[premise_id] != nullptr);

    // check that premise says that {I} is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_id].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule CI: knowledge #" << premise_id
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[dead_knowledge->get_set_id()].get());
    if ((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule CI: knowledge #" << premise_id
                  << " does not state that the constant initial set is dead." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new UnsolvableKnowledge()), conclusion_id);
    unsolvability_proven = true;
    return true;
}

// Conclusion Goal: given (1) S_G^\Pi is dead, the task is unsolvable
bool ProofChecker::check_rule_cg(int conclusion_id, int premise_id) {
    assert(kbentries[premise_id] != nullptr);

    // check that premise says that S_G^\Pi is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_id].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule CG: knowledge #" << premise_id
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[dead_knowledge->get_set_id()].get());
    if ( (!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule CG: knowledge #" << premise_id
                  << " does not state that the constant goal set is dead." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new UnsolvableKnowledge()), conclusion_id);
    unsolvability_proven = true;
    return true;
}

/*
 * SET THEORY RULES
 */

template<class T>
bool ProofChecker::check_rule_ur(int conclusion_id, int left_id, int right_id, std::vector<int> &) {
    T *right = get_set_expression<T>(left_id);
    SetUnion *runion = dynamic_cast<SetUnion *>(right);
    if (!runion) {
        std::cerr << "Error when applying rule UR: right side is not a union" << std::endl;
        return false;
    }
    if (!runion->get_left_id() != left_id) {
        std::cerr << "Error when applying rule UR: right does not have the form (left cup E')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left_id, right_id)), conclusion_id);
    return true;
}

#if 0
bool ProofChecker::check_rule_ul(int conclusion, int left, int right) {
    StateSetUnion *runion = dynamic_cast<StateSetUnion *>(formulas[right].get());
    if (!runion) {
        std::cerr << "Error when applying rule UL: right side is not a union" << std::endl;
        return false;
    }
    if (!runion->get_right_id() != left) {
        std::cerr << "Error when applying rule UL: right does not have the form (E' cup left)" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left, right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_ir(int conclusion, int left, int right) {
    StateSetIntersection *lintersection = dynamic_cast<StateSetIntersection *>(formulas[left].get());
    if (!lintersection) {
        std::cerr << "Error when applying rule IR: left side is not an intersection" << std::endl;
        return false;
    }
    if (!lintersection->get_left_id() != right) {
        std::cerr << "Error when applying rule IR: left does not have the form (right cap E')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_il(int conclusion, int left, int right) {
    StateSetIntersection *lintersection = dynamic_cast<StateSetIntersection *>(formulas[left].get());
    if (!lintersection) {
        std::cerr << "Error when applying rule IL: left side is not an intersection" << std::endl;
        return false;
    }
    if (!lintersection->get_right_id() != right) {
        std::cerr << "Error when applying rule IL: left does not have the form (E' cap right)" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_di(int conclusion, int left, int right) {
    int e0,e1,e2;

    // left side
    StateSetIntersection *si = dynamic_cast<StateSetIntersection *>(formulas[left].get());
    if (!si) {
        std::cerr << "Error when applying rule DI: left is not an intersection" << std::endl;
        return false;
    }
    StateSetUnion *su = dynamic_cast<StateSetUnion *>(formulas[si->get_left_id()].get());
    if (!su) {
        std::cerr << "Error when applying rule DI: left side of left is not a union" << std::endl;
        return false;
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = si->get_right_id();

    // right side
    su = dynamic_cast<StateSetUnion *>(formulas[right].get());
    if (!su) {
        std::cerr << "Error when applying rule DI: right is not a union" << std::endl;
        return false;
    }
    si = dynamic_cast<StateSetIntersection *>(formulas[su->get_left_id()].get());
    if (!si) {
        std::cerr << "Error when applying rule DI: left side of right is not an intersection" << std::endl;
        return false;
    }
    if (si->get_left_id() != e0 || si->get_right_id() != e2) {
        std::cerr << "Error when applying rule DI: left side of right does not match with left" << std::endl;
        return false;
    }
    si = dynamic_cast<StateSetIntersection *>(formulas[su->get_right_id()].get());
    if (!si) {

        std::cerr << "Error when applying rule DI: right side of right is not an intersection" << std::endl;
        return false;
    }
    if (si->get_left_id() != e1 || si->get_right_id() != e2) {
        std::cerr << "Error when applying rule DI: right side of right does not match with left" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_su(int conclusion, int left, int right,
                                 int premise_ids[0], int premise_ids[1]) {
    assert(kbentries[premise_ids[0]] != nullptr && kbentries[premise_ids[1]] != nullptr);
    int e0,e1,e2;
    StateSetUnion *su = dynamic_cast<StateSetUnion *>(formulas[left].get());
    if (!su) {
        std::cerr << "Error when applying rule SU: left is not a union" << std::endl;
        return false;
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = right;

    if (kbentries[premise_ids[0]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[1]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[1] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[0]]->get_first() != e0 || kbentries[premise_ids[0]]->get_second() != e2) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[0] << " does not state (E subset E'')" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[1]]->get_first() != e1 || kbentries[premise_ids[1]]->get_second() != e2) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[0] << " does not state (E' subset E'')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_si(int conclusion, int left, int right,
                                 int premise_ids[0], int premise_ids[1]) {
    assert(kbentries[premise_ids[0]] != nullptr && kbentries[premise_ids[1]] != nullptr);
    int e0,e1,e2;
    StateSetIntersection *si = dynamic_cast<StateSetIntersection*>(formulas[right].get());
    if (!si) {
        std::cerr << "Error when applying rule SI: right is not an intersection" << std::endl;
        return false;
    }
    e0 = left;
    e1 = si->get_left_id();
    e2 = si->get_right_id();

    if (kbentries[premise_ids[0]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[1]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[1] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[0]]->get_first() != e0 || kbentries[premise_ids[0]]->get_second() != e1) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E subset E')" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[1]]->get_first() != e0 || kbentries[premise_ids[1]]->get_second() != e2) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E subset E'')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_st(int conclusion, int left, int right,
                                 int premise_ids[0], int premise_ids[1]) {
    assert(kbentries[premise_ids[0]] != nullptr && kbentries[premise_ids[1]] != nullptr);
    if (kbentries[premise_ids[0]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule ST: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[1]]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule ST: knowledge #" << premise_ids[1] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[0]]->get_first() != left) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E subset E')" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[1]]->get_second() != right) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E subset E'')" << std::endl;
        return false;
    }
    if (kbentries[premise_ids[0]]->get_second() != kbentries[premise_ids[1]]->get_first()) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " and #" << premise_ids[1] << " do not match on E'" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left,right)), conclusion);
    return true;
}
#endif


/*
 * RULES ABOUT PRO/REGRESSION
 */

bool ProofChecker::check_rule_at(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    //TODO
    return false;
}

/*bool ProofChecker::check_rule_au(int conclusion, int left, int right,
                                 int premise_ids[0], int premise_ids[1]) {
    //TODO
    return false;
}

bool ProofChecker::check_rule_pt(int conclusion, int left, int right,
                                 int premise_ids[0], int premise_ids[1]) {
    //TODO
    return false;
}

bool ProofChecker::check_rule_pu(int conclusion, int left, int right,
                                 int premise_ids[0], int premise_ids[1]) {
    //TODO
    return false;
}

// Progression to Regression: given (1) S[A] \subseteq S', then [A]S'_not \subseteq S_not
bool ProofChecker::check_rule_pr(int conclusion, int left, int right,
                                int premise) {
    assert(kbentries[premise] != nullptr);

    //check that left represents [A]S'_not and right S_not
    StateSetRegression *sp_not_reg =
            dynamic_cast<StateSetRegression *>(formulas[left].get());
    if(!sp_not_reg) {
        std::cerr << "Error when applying rule PR: set expression #" << left
                  << " is not a regression." << std::endl;
        return false;
    }
    StateSetNegation *sp_not =
            dynamic_cast<StateSetNegation *>(formulas[sp_not_reg->get_stateset_id()].get());
    if(!sp_not) {
        std::cerr << "Error when applying rule PR: set expression #" << left
                  << " is not the regression of a negation." << std::endl;
        return false;
    }
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(formulas[right].get());
    if(!s_not) {
        std::cerr << "Error when applying rule PR: set expression #" << right
                  << " is not a negation." << std::endl;
        return false;
    }

    int si = s_not->get_child_id();
    int spi = sp_not->get_child_id();

    // check if premise says that S[A] \subseteq S'
    if(kbentries[premise]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule PR: knwoledge #" << premise
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(formulas[kbentries[premise]->get_first()].get());
    if(!s_prog || s_prog->get_stateset_id() != si || kbentries[premise]->get_second() != spi) {
        std::cerr << "Error when applying rule PR: knowledge #" << premise
                  << " does not state that the progression of set expression #" << si
                  << " is a subset of set expression #" << spi << "." << std::endl;
        return false;
    }
    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left, right)), conclusion);
    return true;
}

// Regression to Progression: given (1) [A]S \subseteq S', then S'_not[A] \subseteq S_not
bool ProofChecker::check_rule_rp(int conclusion, int left, int right,
                                int premise) {
    assert(kbentries[premise] != nullptr);

    // check that left represents S'_not[A] and right S_not
    StateSetProgression *sp_not_prog =
            dynamic_cast<StateSetProgression *>(formulas[left].get());
    if(!sp_not_prog) {
        std::cerr << "Error when applying rule RP: set expression #" << left
                  << " is not a progression." << std::endl;
        return false;
    }
    StateSetNegation *sp_not =
            dynamic_cast<StateSetNegation *>(formulas[sp_not_prog->get_stateset_id()].get());
    if(!sp_not) {
        std::cerr << "Error when applying rule RP: set expression #" << left
                  << " is not the progression of a negation." << std::endl;
        return false;
    }
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(formulas[right].get());
    if(!s_not) {
        std::cerr << "Error when applying rule RP: set expression #" << right
                  << " is not a negation." << std::endl;
        return false;
    }

    int si = s_not->get_child_id();
    int spi = sp_not->get_child_id();

    // check if premise says that [A]S \subseteq S'
    if(kbentries[premise]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule RP: knowledge #" << premise
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(formulas[kbentries[premise]->get_first()].get());
    if(!s_reg || s_reg->get_stateset_id() != si || kbentries[premise]->get_second() != spi) {
        std::cerr << "Error when applying rule RP: knowledge #" << premise
                  << " does not state that the regression of set expression #" << si
                  << " is a subset of set expression #" << spi << "." << std::endl;
        return false;
    }
    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(left, right)), conclusion);
    return true;
}
*/


/*
 * BASIC STATEMENTS
 */

// check if \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B1(int newki, int fi1, int fi2, std::vector<int> &) {
    bool ret = false;

    try {
        std::vector<StateSetVariable *> left;
        std::vector<StateSetVariable *> right;

        StateSetVariable *reference_formula = gather_sets_intersection(formulas[fi1].get(), left, right);
        if(!reference_formula) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(fi1)
                    + " is not a intersection of literals of the same type.";
            throw std::runtime_error(msg);
        }
        StateSetVariable *tmp = gather_sets_union(formulas[fi2].get(), right, left);
        if (!tmp) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(fi2)
                    + " is not a union of literals of the same type.";
            throw std::runtime_error(msg);
        }
        reference_formula =
                update_reference_and_check_consistency(reference_formula, tmp, "B1");

        if (!reference_formula->check_statement_b1(left, right)) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(fi1) + " is not a subset of set expression #"
                    + std::to_string(fi2) + ".";
            throw std::runtime_error(msg);
        }

        add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(fi1,fi2)), newki);
        ret = true;
    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}

#if 0
// check if (\bigcap_{X \in \mathcal X} X)[A] \land \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B2(int newki, int fi1, int fi2) {
    bool ret = false;

    try {
        std::vector<StateSetVariable *> prog;
        std::vector<StateSetVariable *> left;
        std::vector<StateSetVariable *> right;
        std::unordered_set<int> actions;
        StateSet *prog_formula = nullptr;
        StateSet *left_formula = nullptr;

        SetFormulaType left_type = formulas[fi1].get()->get_formula_type();
        /*
         * We expect the left side to either be a progression or an intersection with
         * a progression on the left side
         */
        if (left_type == SetFormulaType::INTERSECTION) {
            StateSetIntersection *intersection =
                    dynamic_cast<StateSetIntersection *>(formulas[fi1].get());
            prog_formula = formulas[intersection->get_left_id()].get();
            left_formula = formulas[intersection->get_right_id()].get();
        } else if (left_type == SetFormulaType::PROGRESSION) {
            prog_formula = formulas[fi1].get();
        }
        // here, prog_formula should be S[A]
        if (!prog_formula || prog_formula->get_formula_type() != SetFormulaType::PROGRESSION) {
            std::string msg = "Error when checking statement B2: set expression #"
                    + std::to_string(fi1)
                    + " is not a progresison or intersection with progression on the left.";
            throw std::runtime_error(msg);
        }
        StateSetProgression *progression = dynamic_cast<StateSetProgression *>(prog_formula);
        actionsets[progression->get_actionset_id()]->get_actions(actions);
        prog_formula = formulas[progression->get_stateset_id()].get();
        // here, prog_formula is S (without [A])

        /*
         * In the progression, we only allow set variables, not set literals.
         * We abuse left here and know it will stay empty if the progression does
         * not contain set literals.
         */
        StateSetVariable *reference_formula = gather_sets_intersection(prog_formula, prog, left);
        if(!reference_formula || !left.empty()) {
            std::string msg = "Error when checking statement B2: "
                              "the progression in set expression #"
                    + std::to_string(fi1)
                    + " is not an intersection of set variables.";
            throw std::runtime_error(msg);
        }

        // left_formula is empty if the left side contains only a progression
        if(left_formula) {
            StateSetVariable *tmp = gather_sets_intersection(left_formula, left, right);
            if(!tmp) {
                std::string msg = "Error when checking statement B2: "
                                  "the non-progression part in set expression #"
                        + std::to_string(fi1)
                        + " is not an intersection of set literals.";
                throw std::runtime_error(msg);
            }
            reference_formula =
                    update_reference_and_check_consistency(reference_formula, tmp, "B2");
        }
        StateSetVariable *tmp = gather_sets_union(formulas[fi2].get(), right, left);
        if(!tmp) {
            std::string msg = "Error when checking statement B2: set expression #"
                    + std::to_string(fi2)
                    + " is not a union of literals.";
            throw std::runtime_error(msg);
        }
        reference_formula =
                update_reference_and_check_consistency(reference_formula, tmp, "B2");

        if(!reference_formula->check_statement_b2(prog, left, right, actions)) {
            std::string msg = "Error when checking statement B2: set expression #"
                    + std::to_string(fi1) + " is not a subset of set expression #"
                    + std::to_string(fi2) + ".";
            throw std::runtime_error(msg);
        }
        add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(fi1,fi2)), newki);
        ret = true;

    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}

// check if [A](\bigcap_{X \in \mathcal X} X) \land \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B3(int newki, int fi1, int fi2) {
    bool ret = false;

    try {
        std::vector<StateSetVariable *> reg;
        std::vector<StateSetVariable *> left;
        std::vector<StateSetVariable *> right;
        std::unordered_set<int> actions;
        StateSet *reg_formula = nullptr;
        StateSet *left_formula = nullptr;

        SetFormulaType left_type = formulas[fi1].get()->get_formula_type();
        /*
         * We expect the left side to either be a regression or an intersection with
         * a regression on the left side
         */
        if (left_type == SetFormulaType::INTERSECTION) {
            StateSetIntersection *intersection =
                    dynamic_cast<StateSetIntersection *>(formulas[fi1].get());
            reg_formula = formulas[intersection->get_left_id()].get();
            left_formula = formulas[intersection->get_right_id()].get();
        } else if (left_type == SetFormulaType::REGRESSION) {
            reg_formula = formulas[fi1].get();
        }
        // here, reg_formula should be [A]S
        if (!reg_formula || reg_formula->get_formula_type() != SetFormulaType::REGRESSION) {
            std::string msg = "Error when checking statement B3: set expression #"
                    + std::to_string(fi1)
                    + " is not a regresison or intersection with regression on the left.";
            throw std::runtime_error(msg);
        }
        StateSetRegression *regression = dynamic_cast<StateSetRegression *>(reg_formula);
        actionsets[regression->get_actionset_id()]->get_actions(actions);
        reg_formula = formulas[regression->get_stateset_id()].get();
        // here, reg_formula is S (without [A])

        /*
         * In the regression, we only allow set variables, not set literals.
         * We abuse left here and know it will stay empty if the regression does
         * not contain set literals.
         */
        StateSetVariable *reference_formula = gather_sets_intersection(reg_formula, reg, left);
        if(!reference_formula || !left.empty()) {
            std::string msg = "Error when checking statement B3: "
                              "the regression in set expression #"
                    + std::to_string(fi1)
                    + " is not an intersection of set variables.";
            throw std::runtime_error(msg);
        }

        // left_formula is empty if the left side contains only a progression
        if(left_formula) {
            StateSetVariable *tmp = gather_sets_intersection(left_formula, left, right);
            if(!tmp) {
                std::string msg = "Error when checking statement B3: "
                                  "the non-regression part in set expression #"
                        + std::to_string(fi1)
                        + " is not an intersection of set literals.";
                throw std::runtime_error(msg);
            }
            reference_formula =
                    update_reference_and_check_consistency(reference_formula, tmp, "B2");
        }
        StateSetVariable *tmp = gather_sets_union(formulas[fi2].get(), right, left);
        if(!tmp) {
            std::string msg = "Error when checking statement B3: set expression #"
                    + std::to_string(fi2)
                    + " is not a union of literals.";
            throw std::runtime_error(msg);
        }
        reference_formula =
                update_reference_and_check_consistency(reference_formula, tmp, "B2");

        if(!reference_formula->check_statement_b3(reg, left, right, actions)) {
            std::string msg = "Error when checking statement B3: set expression #"
                    + std::to_string(fi1) + " is not a subset of set expression #"
                    + std::to_string(fi2) + ".";
            throw std::runtime_error(msg);
        }
        add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(fi1,fi2)), newki);
        ret = true;

    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}


// check if L \subseteq L', where L and L' might be represented by different formalisms
bool ProofChecker::check_statement_B4(int newki, int fi1, int fi2) {
    bool ret = true;

    try {
        StateSet *left = formulas[fi1].get();
        bool left_positive = true;
        StateSet *right = formulas[fi2].get();
        bool right_positive = true;

        if(left->get_formula_type() == SetFormulaType::NEGATION) {
            StateSetNegation *f1neg = dynamic_cast<StateSetNegation *>(left);
            left = formulas[f1neg->get_child_id()].get();
            left_positive = false;
        }
        StateSetVariable *lvar = dynamic_cast<StateSetVariable *>(left);
        if (!lvar) {
            std::string msg = "Error when checking statement B4: set expression #"
                    + std::to_string(fi1) + " is not a set literal.";
            throw std::runtime_error(msg);
        }

        if(right->get_formula_type() == SetFormulaType::NEGATION) {
            StateSetNegation *f2neg = dynamic_cast<StateSetNegation *>(right);
            right = formulas[f2neg->get_child_id()].get();
            right_positive = false;
        }
        StateSetVariable *rvar = dynamic_cast<StateSetVariable *>(right);
        if (!rvar) {
            std::string msg = "Error when checking statement B4: set expression #"
                    + std::to_string(fi2) + " is not a set literal.";
            throw std::runtime_error(msg);
        }

        if(!lvar->check_statement_b4(rvar, left_positive, right_positive)) {
            std::string msg = "Error when checking statement B4: set expression #"
                    + std::to_string(fi1) + " is not a subset of set expression #"
                    + std::to_string(fi2) + ".";
            throw std::runtime_error(msg);
        }

        add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge(fi1,fi2)), newki);
    } catch (std::runtime_error e) {
        std::cerr << e.what();
        ret = false;
    }
    return ret;
}


// check if A \subseteq A'
bool ProofChecker::check_statement_B5(int newki,
                                      int ai1,
                                      int ai2) {
    if(!actionsets[ai1].get()->is_subset(actionsets[ai2].get())) {
        std::cerr << "Error when checking statement B5: action set #"
                  << ai1 << " is not a subset of action set #" << ai2 << "." << std::endl;
        return false;
    } else {
        add_knowledge(std::unique_ptr<KBEntry>(new KBEntryAction(ai1,ai2)), newki);
        return true;
    }
}
#endif


bool ProofChecker::is_unsolvability_proven() {
    return unsolvability_proven;
}
