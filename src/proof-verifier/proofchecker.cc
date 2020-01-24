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

KBEntry::KBEntry() {

}

ProofChecker::ProofChecker()
    : unsolvability_proven(false) {
    // build maps from shorthand used in certificate file to enum type
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

}

void ProofChecker::add_kbentry(std::unique_ptr<KBEntry> entry, KnowledgeIndex index) {
    assert(index >= kbentries.size());
    if(index > kbentries.size()) {
        kbentries.resize(index);
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
        f = formulas[dynamic_cast<StateSetNegation *>(f)->get_child_id()].fpointer.get();
        ret = gather_sets_intersection(f, negative, positive);
        break;
    case SetFormulaType::INTERSECTION: {
        StateSetIntersection *fi = dynamic_cast<StateSetIntersection *>(f);
        ret = gather_sets_intersection(formulas[fi->get_left_id()].fpointer.get(),
                positive, negative);
        if(ret) {
            StateSetVariable *ret2 = gather_sets_intersection(formulas[fi->get_right_id()].fpointer.get(),
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
        f = formulas[dynamic_cast<StateSetNegation *>(f)->get_child_id()].fpointer.get();
        ret = gather_sets_union(f, negative, positive);
        break;
    case SetFormulaType::UNION: {
        StateSetUnion *fi = dynamic_cast<StateSetUnion *>(f);
        ret = gather_sets_union(formulas[fi->get_left_id()].fpointer.get(),
                positive, negative);
        if(ret) {
            StateSetVariable *ret2 = gather_sets_union(formulas[fi->get_right_id()].fpointer.get(),
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

void ProofChecker::add_expression(std::ifstream &in, Task *task) {

    FormulaIndex expression_index;
    in >> expression_index;
    std::string word;
    // read in expression type
    in >> word;
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
        expression = std::unique_ptr<StateSet>(new SetFormulaBDD(in, task));
        break;
    case ExpressionType::HORN:
        expression = std::unique_ptr<StateSet>(new SetFormulaHorn(in, task));
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
        expression = std::unique_ptr<StateSet>(new SetFormulaExplicit(in, task));
        break;
    case ExpressionType::CONSTANT:
        expression = std::unique_ptr<StateSet>(new SetFormulaConstant(in, task));
        break;
    case ExpressionType::NEGATION: {
        FormulaIndex subformulaindex;
        in >> subformulaindex;
        expression = std::unique_ptr<StateSet>(new StateSetNegation(subformulaindex));
        break;
    }
    case ExpressionType::INTERSECTION: {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        expression = std::unique_ptr<StateSet>(new StateSetIntersection(left, right));
        break;
    }
    case ExpressionType::UNION: {
        FormulaIndex left, right;
        in >> left;
        in >> right;
        expression = std::unique_ptr<StateSet>(new StateSetUnion(left, right));
        break;
    }
    case ExpressionType::PROGRESSION: {
        FormulaIndex subformulaindex, actionsetindex;
        in >> subformulaindex;
        in >> actionsetindex;
        expression = std::unique_ptr<StateSet>(new StateSetProgression(subformulaindex, actionsetindex));
        break;
    }
    case ExpressionType::REGRESSION: {
        FormulaIndex subformulaindex, actionsetindex;
        in >> subformulaindex;
        in >> actionsetindex;
        expression = std::unique_ptr<StateSet>(new StateSetRegression(subformulaindex, actionsetindex));
        break;
    }
    default:
        std::cerr << "unknown expression type " << word << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    // if g_discard_formulas, first_pass() will guarantee that the entry for this index exists already
    if (!g_discard_formulas && expression_index >= formulas.size()) {
        formulas.resize(expression_index+1);
    }
    assert(!formulas[expression_index].fpointer);
    formulas[expression_index].fpointer = std::move(expression);
}

void ProofChecker::add_actionset(std::unique_ptr<ActionSet> actionset, ActionSetIndex index) {
    assert(index >= actionsets.size());
    if(index > actionsets.size()) {
        actionsets.resize(index);
    }
    actionsets.push_back(std::move(actionset));
}

void ProofChecker::add_actionset_union(ActionSetIndex left, ActionSetIndex right, ActionSetIndex index) {
    assert(index >= actionsets.size());
    if(index > actionsets.size()) {
        actionsets.resize(index);
    }
    actionsets.push_back(std::unique_ptr<ActionSet>(
                             new ActionSetUnion(actionsets[left].get(),
                                                actionsets[right].get())));
}

void ProofChecker::remove_formulas_if_obsolete(std::vector<int> indices, int current_ki) {
    for(int index: indices) {
        if(formulas[index].last_occ == current_ki) {
            switch (formulas[index].fpointer.get()->get_formula_type()) {
            case SetFormulaType::BDD:
            case SetFormulaType::HORN:
            case SetFormulaType::TWOCNF:
            case SetFormulaType::EXPLICIT:
                formulas[index].fpointer.reset();
                break;
            case SetFormulaType::NEGATION: {
                StateSetNegation *f = dynamic_cast<StateSetNegation *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_child_id()}, current_ki);
                break;
            }
            case SetFormulaType::PROGRESSION: {
                StateSetProgression *f = dynamic_cast<StateSetProgression *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_stateset_id()}, current_ki);
                break;
            }
            case SetFormulaType::REGRESSION: {
                StateSetRegression *f = dynamic_cast<StateSetRegression *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_stateset_id()}, current_ki);
                break;
            }
            case SetFormulaType::INTERSECTION: {
                StateSetIntersection *f = dynamic_cast<StateSetIntersection *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_left_id(), f->get_right_id()}, current_ki);
                break;
            }
            case SetFormulaType::UNION: {
                StateSetUnion *f = dynamic_cast<StateSetUnion *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_left_id(), f->get_right_id()}, current_ki);
                break;
            }
            }

        }
    }
}

/*
 * This method goes over the entire certificate file and collects information
 * on when the actual representation of basic sets are needed last.
 * We gather this information by checking knowledge gained through B1-B5 (since
 * only those require the actual representation). Since the declaration of knowledge
 * gained through B2-B5 involves compound sets we also store information on those.
 * However, that information should only be used for getting the real last_occ value for
 * basic sets; the compound set may still be needed later on.
 */
void ProofChecker::first_pass(std::string certfile) {
    std::deque<std::pair<int, std::pair<int,int>>> compound_sets;
    std::ifstream certstream;
    certstream.open(certfile);
    std::string input;
    int mainsetid, set1, set2, kid;
    std::vector<int> constant_formulas;

    while(certstream >> input) {

        if(input == "e") {
            certstream >> mainsetid;
            certstream >> input;
            // compound set with 2 subsets (intersection & union)
            if(input == "i" || input == "u") {
                certstream >> set1;
                certstream >> set2;
                compound_sets.push_back(std::make_pair(mainsetid, std::make_pair(set1,set2)));
                // we don't need to skip to next line since we are at the end already
            // compound set with 1 subset (progression, regression & negation)
            } else if(input == "p" || input == "r" || input == "n") {
                certstream >> set1;
                compound_sets.push_back(std::make_pair(mainsetid, std::make_pair(set1, -1)));
                // we don't need to skip to next line since we are at the end already
            } else {
                if(input == "c") {
                    constant_formulas.push_back(mainsetid);
                }
                // skip to next line
                certstream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            if(mainsetid >= formulas.size()) {
                formulas.resize(mainsetid+1);
            }
        } else if(input == "k") {
            certstream >> kid;
            certstream >> input;
            /* only subset knowledge potentially needs the actual sets
             * (if the subset is proven via B1-B5)
             */
            if (input == "s") {
                certstream >> set1;
                certstream >> set2;
                certstream >> input;
                if(input.at(0) == 'b') {
                    formulas[set1].last_occ = kid;
                    formulas[set2].last_occ = kid;
                    continue;
                }
            }
            // skip to next line
            certstream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    certstream.close();

    /* Set the last usage of formulas that occur as subformula as the max of it's last occurence
     * and the last occurence of the compound formula.
     * Note: since compound formulas are declared bottom up, a reverse iteration will guarantee
     * that the max-calculation is done top down and thus correct.
     */
    // TODO: this assumes that indices are used in ascending order - is this ok?
    int cset, sset1, sset2;
    for (auto rit = compound_sets.rbegin(); rit!= compound_sets.rend(); ++rit) {
        cset = rit->first;
        sset1 = rit->second.first;
        sset2 = rit->second.second;
        formulas[sset1].last_occ = std::max(formulas[sset1].last_occ, formulas[cset].last_occ);
        if(sset2 >= 0) {
            formulas[sset2].last_occ = std::max(formulas[sset2].last_occ, formulas[cset].last_occ);
        }
    }


    // constant formulas should never be deleted
    for(size_t i = 0; i < constant_formulas.size(); ++i) {
        formulas[constant_formulas[i]].last_occ = -1;
    }
}

// TODO: unify error messages

/*
 * RULES ABOUT DEADNESS
 */

// Emptyset Dead: set=emptyset is dead
bool ProofChecker::check_rule_ed(KnowledgeIndex conclusion, FormulaIndex set) {

    SetFormulaConstant *f = dynamic_cast<SetFormulaConstant *>(formulas[set].fpointer.get());
    if ((!f) || (f->get_constant_type() != ConstantType::EMPTY)) {
        std::cerr << "Error when applying rule ED: set expression #" << set
                  << " is not the constant empty set." << std::endl;
        return false;
    }
    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(set)), conclusion);
    return true;
}


// Union Dead: given (1) S is dead and (2) S' is dead, set=S \cup S' is dead
bool ProofChecker::check_rule_ud(KnowledgeIndex conclusion, FormulaIndex set,
                               KnowledgeIndex premise1, KnowledgeIndex premise2) {
    assert(kbentries[premise1] && kbentries[premise2]);

    // set represents S \cup S'
    StateSetUnion *f = dynamic_cast<StateSetUnion *>(formulas[set].fpointer.get());
    if (!f) {
        std::cerr << "Error when applying rule UD to conclude knowledge #" << conclusion
                  << ": set expression #" << set << "is not a union." << std::endl;
        return false;
    }
    FormulaIndex lefti = f->get_left_id();
    FormulaIndex righti = f->get_right_id();

    // check if premise1 says that S is dead
    if ((kbentries[premise1].get()->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[premise1].get()->get_first() != lefti)) {
        std::cerr << "Error when applying rule UD: Knowledge #" << premise1
                  << "does not state that set expression #" << lefti
                  << " is dead." << std::endl;
        return false;
    }

    // check if premise2 says that S' is dead
    if ((kbentries[premise2].get()->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[premise2].get()->get_first() != righti)) {
        std::cerr << "Error when applying rule UD: Knowledge #" << premise2
                  << "does not state that set expression #" << righti
                  << " is dead." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(set)), conclusion);
    return true;
}


// Subset Dead: given (1) S \subseteq S' and (2) S' is dead, set=S is dead
bool ProofChecker::check_rule_sd(KnowledgeIndex conclusion, FormulaIndex set,
                               KnowledgeIndex premise1, KnowledgeIndex premise2) {
    assert(kbentries[premise1] != nullptr && kbentries[premise2] != nullptr);

    // check if premise1 says that S is a subset of S' (S' can be anything)
    if ((kbentries[premise1]->get_kbentry_type() != KBType::SUBSET) ||
       (kbentries[premise1]->get_first() != set)) {
        std::cerr << "Error when applying rule SD: knowledge #" << premise1
                  << " does not state that set expression #" << set
                  << " is a subset of another set." << std::endl;
        return false;
    }

    FormulaIndex xi = kbentries[premise1]->get_second();

    // check if premise2 says that S' is dead
    if ((kbentries[premise2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise2]->get_first() != xi)) {
        std::cerr << "Error when applying rule SD: knowledge #" << premise1
                  << " states that set expression #" << set
                  << " is a subset of set expression #" << xi << ", but knowledge #" << premise2
                  << " does not state that " << xi << " is dead." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(set)), conclusion);
    return true;
}

// Progression Goal: given (1) S[A] \subseteq S \cup S', (2) S' is dead and (3) S \cap S_G^\Pi is dead,
// then set=S is dead
bool ProofChecker::check_rule_pg(KnowledgeIndex conclusion, FormulaIndex set,
                               KnowledgeIndex premise1, KnowledgeIndex premise2, KnowledgeIndex premise3) {
    assert(kbentries[premise1] != nullptr && kbentries[premise2] != nullptr && kbentries[premise3] != nullptr);

    // check if premise1 says that S[A] \subseteq S \cup S'
    if (kbentries[premise1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise1
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise1 is S[A]
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(formulas[kbentries[premise1]->get_first()].fpointer.get());
    if ((!s_prog) || (s_prog->get_stateset_id() != set)) {
        std::cerr << "Error when applying rule PG: the left side of subset knowledge #" << premise1
                  << " is not the progression of set expression #" << set << "." << std::endl;
        return false;
    }
    if(!actionsets[s_prog->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule PG: "
                     "the progression does not speak about all actions" << std::endl;
        return false;
    }
    // check if the right side of premise1 is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(formulas[kbentries[premise1]->get_second()].fpointer.get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != set)) {
        std::cerr << "Error when applying rule PG: the right side of subset knowledge #" << premise1
                  << " is not a union of set expression #" << set
                  << " and another set expression." << std::endl;
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_id();

    // check if premise2 says that S' is dead
    if ((kbentries[premise2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise2]->get_first() != spi)) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise2
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if premise3 says that S \cap S_G^\Pi is dead
    if (kbentries[premise3]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise3
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    StateSetIntersection *s_and_goal =
            dynamic_cast<StateSetIntersection *>(formulas[kbentries[premise3]->get_first()].fpointer.get());
    // check if left side of s_and_goal is S
    if ((!s_and_goal) || (s_and_goal->get_left_id() != set)) {
        std::cerr << "Error when applying rule PG: the set expression declared dead in knowledge #"
                  << premise3 << " is not an intersection with set expression #" << set
                  << " on the left side." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_and_goal->get_right_id()].fpointer.get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule PG: the set expression declared dead in knowledge #"
                  << premise3 << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(set)), conclusion);
    return true;
}


// Progression Initial: given (1) S[A] \subseteq S \cup S', (2) S' is dead and (3) {I} \subseteq S_not,
// then set=S_not is dead
bool ProofChecker::check_rule_pi(KnowledgeIndex conclusion, FormulaIndex set,
                               KnowledgeIndex premise1, KnowledgeIndex premise2, KnowledgeIndex premise3) {
    assert(kbentries[premise1] != nullptr && kbentries[premise2] != nullptr && kbentries[premise3] != nullptr);

    // check if set corresponds to s_not
    StateSetNegation *s_not = dynamic_cast<StateSetNegation *>(formulas[set].fpointer.get());
    if(!s_not) {
        std::cerr << "Error when applying rule PI: set expression #" << set
                  << " is not a negation." << std::endl;
        return false;
    }
    FormulaIndex si = s_not->get_child_id();

    // check if premise1 says that S[A] \subseteq S \cup S'
    if (kbentries[premise1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise1
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise1 is S[A]
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(formulas[kbentries[premise1]->get_first()].fpointer.get());
    if ((!s_prog) || (s_prog->get_stateset_id() != si)) {
        std::cerr << "Error when applying rule PI: the left side of subset knowledge #" << premise1
                  << " is not the progression of set expression #" << si << "." << std::endl;
        return false;
    }
    if(!actionsets[s_prog->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule PI: "
                     "the progression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise1 is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(formulas[kbentries[premise1]->get_second()].fpointer.get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != si)) {
        std::cerr << "Error when applying rule PI: the right side of subset knowledge #" << premise1
                  << " is not a union of set expression #" << si
                  << " and another set expression." << std::endl;
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_id();

    // check if premise2 says that S' is dead
    if ((kbentries[premise2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise2]->get_first() != spi)) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise2
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if premise3 says that {I} \subseteq S
    if (kbentries[premise3]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise3
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of premise3 is {I}
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[premise3]->get_first()].fpointer.get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule PI: the left side of subset knowledge #" << premise3
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of pemise3 is S
    if(kbentries[premise3]->get_second() != si) {
        std::cerr << "Error when applying rule PI: the right side of subset knowledge #" << premise3
                  << " is not set expression #" << si << "." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(set)), conclusion);
    return true;
}


// Regression Goal: given (1)[A]S \subseteq S \cup S', (2) S' is dead and (3) S_not \cap S_G^\Pi is dead,
// then set=S_not is dead
bool ProofChecker::check_rule_rg(KnowledgeIndex conclusion, FormulaIndex set,
                               KnowledgeIndex premise1, KnowledgeIndex premise2, KnowledgeIndex premise3) {
    assert(kbentries[premise1] != nullptr && kbentries[premise2] != nullptr && kbentries[premise3] != nullptr);

    // check if set corresponds to s_not
    StateSetNegation *s_not = dynamic_cast<StateSetNegation *>(formulas[set].fpointer.get());
    if(!s_not) {
        std::cerr << "Error when applying rule RG: set expression #" << set
                  << " is not a negation." << std::endl;
        return false;
    }
    FormulaIndex si = s_not->get_child_id();

    // check if premise1 says that [A]S \subseteq S \cup S'
    if(kbentries[premise1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise1
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise1 is [A]S
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(formulas[kbentries[premise1]->get_first()].fpointer.get());
    if ((!s_reg) || (s_reg->get_stateset_id() != si)) {
        std::cerr << "Error when applying rule RG: the left side of subset knowledge #" << premise1
                  << " is not the regression of set expression #" << si << "." << std::endl;
        return false;
    }
    if(!actionsets[s_reg->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule RG: "
                     "the regression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise1 is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(formulas[kbentries[premise1]->get_second()].fpointer.get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != si)) {
        std::cerr << "Error when applying rule RG: the right side of subset knowledge #" << premise1
                  << " is not a union of set expression #" << si
                  << " and another set expression." << std::endl;
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_id();

    // check if premise2 says that S' is dead
    if ((kbentries[premise2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise2]->get_first() != spi)) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise2
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if premise3 says that S_not \cap S_G(\Pi) is dead
    if (kbentries[premise3]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise3
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    StateSetIntersection *s_not_and_goal =
            dynamic_cast<StateSetIntersection *>(formulas[kbentries[premise3]->get_first()].fpointer.get());
    // check if left side of s_not_and goal is S_not
    if ((!s_not_and_goal) || (s_not_and_goal->get_left_id() != set)) {
        std::cerr << "Error when applying rule RG: the set expression declared dead in knowledge #"
                  << premise3 << " is not an intersection with set expression #" << set
                  << " on the left side." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_not_and_goal->get_right_id()].fpointer.get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule RG: the set expression declared dead in knowledge #"
                  << premise3 << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(set)), conclusion);
    return true;
}


// Regression Initial: (1) [A]S \subseteq S \cup S', (2) S' is dead and (3) {I} \subseteq S_not,
// then set=S is dead
bool ProofChecker::check_rule_ri(KnowledgeIndex conclusion, FormulaIndex set,
                               KnowledgeIndex premise1, KnowledgeIndex premise2, KnowledgeIndex premise3) {
    assert(kbentries[premise1] != nullptr && kbentries[premise2] != nullptr && kbentries[premise3] != nullptr);

    // check if premise1 says that [A]S \subseteq S \cup S'
    if(kbentries[premise1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise1
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise1 is [A]S
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(formulas[kbentries[premise1]->get_first()].fpointer.get());
    if ((!s_reg) || (s_reg->get_stateset_id() != set)) {
        std::cerr << "Error when applying rule RI: the left side of subset knowledge #" << premise1
                  << " is not the regression of set expression #" << set << "." << std::endl;
        return false;
    }
    if(!actionsets[s_reg->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule RI: "
                     "the regression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise1 is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(formulas[kbentries[premise1]->get_second()].fpointer.get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != set)) {
        std::cerr << "Error when applying rule RI: the right side of subset knowledge #" << premise1
                  << " is not a union of set expression #" << set
                  << " and another set expression." << std::endl;
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_id();

    // check if k2 says that S' is dead
    if ((kbentries[premise2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[premise2]->get_first() != spi)) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise2
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if premise3 says that {I} \subseteq S_not
    if (kbentries[premise3]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise3
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of premise3 is {I}
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[premise3]->get_first()].fpointer.get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule RI: the left side of subset knowledge #" << premise3
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of premise3 is S_not
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(formulas[kbentries[premise3]->get_second()].fpointer.get());
    if((!s_not) || s_not->get_child_id() != set) {
        std::cerr << "Error when applying rule RI: the right side of subset knowledge #" << premise3
                  << " is not the negation of set expression #" << set << "." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(set)), conclusion);
    return true;
}


/*
 * CONCLUSION RULES
 */

// Conclusion Initial: given (1) {I} is dead, the task is unsolvable
bool ProofChecker::check_rule_ci(KnowledgeIndex conclusion, KnowledgeIndex premise) {
    assert(kbentries[premise] != nullptr);

    // check that premise says that {I} is dead
    if (kbentries[premise]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule CI: knowledge #" << premise
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[premise]->get_first()].fpointer.get());
    if ((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule CI: knowledge #" << premise
                  << " does not state that the constant initial set is dead." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryUnsolvable()), conclusion);
    unsolvability_proven = true;
    return true;
}

// Conclusion Goal: given (1) S_G^\Pi is dead, the task is unsolvable
bool ProofChecker::check_rule_cg(KnowledgeIndex conclusion, KnowledgeIndex premise) {
    assert(kbentries[premise] != nullptr);

    // check that premise says that S_G^\Pi is dead
    if (kbentries[premise]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule CG: knowledge #" << premise
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[premise]->get_first()].fpointer.get());
    if ( (!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule CG: knowledge #" << premise
                  << " does not state that the constant goal set is dead." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryUnsolvable()), conclusion);
    unsolvability_proven = true;
    return true;
}

/*
 * SET THEORY RULES
 */

bool ProofChecker::check_rule_ur(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right) {
    StateSetUnion *runion = dynamic_cast<StateSetUnion *>(formulas[right].fpointer.get());
    if (!runion) {
        std::cerr << "Error when applying rule UR: right side is not a union" << std::endl;
        return false;
    }
    if (!runion->get_left_id() != left) {
        std::cerr << "Error when applying rule UR: right does not have the form (left cup E')" << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left, right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_ul(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right) {
    StateSetUnion *runion = dynamic_cast<StateSetUnion *>(formulas[right].fpointer.get());
    if (!runion) {
        std::cerr << "Error when applying rule UL: right side is not a union" << std::endl;
        return false;
    }
    if (!runion->get_right_id() != left) {
        std::cerr << "Error when applying rule UL: right does not have the form (E' cup left)" << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left, right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_ir(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right) {
    StateSetIntersection *lintersection = dynamic_cast<StateSetIntersection *>(formulas[left].fpointer.get());
    if (!lintersection) {
        std::cerr << "Error when applying rule IR: left side is not an intersection" << std::endl;
        return false;
    }
    if (!lintersection->get_left_id() != right) {
        std::cerr << "Error when applying rule IR: left does not have the form (right cap E')" << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_il(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right) {
    StateSetIntersection *lintersection = dynamic_cast<StateSetIntersection *>(formulas[left].fpointer.get());
    if (!lintersection) {
        std::cerr << "Error when applying rule IL: left side is not an intersection" << std::endl;
        return false;
    }
    if (!lintersection->get_right_id() != right) {
        std::cerr << "Error when applying rule IL: left does not have the form (E' cap right)" << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_di(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right) {
    FormulaIndex e0,e1,e2;

    // left side
    StateSetIntersection *si = dynamic_cast<StateSetIntersection *>(formulas[left].fpointer.get());
    if (!si) {
        std::cerr << "Error when applying rule DI: left is not an intersection" << std::endl;
        return false;
    }
    StateSetUnion *su = dynamic_cast<StateSetUnion *>(formulas[si->get_left_id()].fpointer.get());
    if (!su) {
        std::cerr << "Error when applying rule DI: left side of left is not a union" << std::endl;
        return false;
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = si->get_right_id();

    // right side
    su = dynamic_cast<StateSetUnion *>(formulas[right].fpointer.get());
    if (!su) {
        std::cerr << "Error when applying rule DI: right is not a union" << std::endl;
        return false;
    }
    si = dynamic_cast<StateSetIntersection *>(formulas[su->get_left_id()].fpointer.get());
    if (!si) {
        std::cerr << "Error when applying rule DI: left side of right is not an intersection" << std::endl;
        return false;
    }
    if (si->get_left_id() != e0 || si->get_right_id() != e2) {
        std::cerr << "Error when applying rule DI: left side of right does not match with left" << std::endl;
        return false;
    }
    si = dynamic_cast<StateSetIntersection *>(formulas[su->get_right_id()].fpointer.get());
    if (!si) {

        std::cerr << "Error when applying rule DI: right side of right is not an intersection" << std::endl;
        return false;
    }
    if (si->get_left_id() != e1 || si->get_right_id() != e2) {
        std::cerr << "Error when applying rule DI: right side of right does not match with left" << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_su(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                 KnowledgeIndex premise1, KnowledgeIndex premise2) {
    assert(kbentries[premise1] != nullptr && kbentries[premise2] != nullptr);
    FormulaIndex e0,e1,e2;
    StateSetUnion *su = dynamic_cast<StateSetUnion *>(formulas[left].fpointer.get());
    if (!su) {
        std::cerr << "Error when applying rule SU: left is not a union" << std::endl;
        return false;
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = right;

    if (kbentries[premise1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise1 << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise2]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise2 << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise1]->get_first() != e0 || kbentries[premise1]->get_second() != e2) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise1 << " does not state (E subset E'')" << std::endl;
        return false;
    }
    if (kbentries[premise2]->get_first() != e1 || kbentries[premise2]->get_second() != e2) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise1 << " does not state (E' subset E'')" << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_si(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                 KnowledgeIndex premise1, KnowledgeIndex premise2) {
    assert(kbentries[premise1] != nullptr && kbentries[premise2] != nullptr);
    FormulaIndex e0,e1,e2;
    StateSetIntersection *si = dynamic_cast<StateSetIntersection*>(formulas[right].fpointer.get());
    if (!si) {
        std::cerr << "Error when applying rule SI: right is not an intersection" << std::endl;
        return false;
    }
    e0 = left;
    e1 = si->get_left_id();
    e2 = si->get_right_id();

    if (kbentries[premise1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise1 << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise2]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise2 << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise1]->get_first() != e0 || kbentries[premise1]->get_second() != e1) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise1 << " does not state (E subset E')" << std::endl;
        return false;
    }
    if (kbentries[premise2]->get_first() != e0 || kbentries[premise2]->get_second() != e2) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise1 << " does not state (E subset E'')" << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left,right)), conclusion);
    return true;
}

bool ProofChecker::check_rule_st(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                 KnowledgeIndex premise1, KnowledgeIndex premise2) {
    assert(kbentries[premise1] != nullptr && kbentries[premise2] != nullptr);
    if (kbentries[premise1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule ST: knowledge #" << premise1 << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise2]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule ST: knowledge #" << premise2 << " is not subset knowledge" << std::endl;
        return false;
    }
    if (kbentries[premise1]->get_first() != left) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise1 << " does not state (E subset E')" << std::endl;
        return false;
    }
    if (kbentries[premise2]->get_second() != right) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise1 << " does not state (E subset E'')" << std::endl;
        return false;
    }
    if (kbentries[premise1]->get_second() != kbentries[premise2]->get_first()) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise1 << " and #" << premise2 << " do not match on E'" << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left,right)), conclusion);
    return true;
}



/*
 * RULES ABOUT PRO/REGRESSION
 */

bool ProofChecker::check_rule_at(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                 KnowledgeIndex premise1, KnowledgeIndex premise2) {
    //TODO
    return false;
}

bool ProofChecker::check_rule_au(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                 KnowledgeIndex premise1, KnowledgeIndex premise2) {
    //TODO
    return false;
}

bool ProofChecker::check_rule_pt(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                 KnowledgeIndex premise1, KnowledgeIndex premise2) {
    //TODO
    return false;
}

bool ProofChecker::check_rule_pu(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                 KnowledgeIndex premise1, KnowledgeIndex premise2) {
    //TODO
    return false;
}

// Progression to Regression: given (1) S[A] \subseteq S', then [A]S'_not \subseteq S_not
bool ProofChecker::check_rule_pr(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                KnowledgeIndex premise) {
    assert(kbentries[premise] != nullptr);

    //check that left represents [A]S'_not and right S_not
    StateSetRegression *sp_not_reg =
            dynamic_cast<StateSetRegression *>(formulas[left].fpointer.get());
    if(!sp_not_reg) {
        std::cerr << "Error when applying rule PR: set expression #" << left
                  << " is not a regression." << std::endl;
        return false;
    }
    StateSetNegation *sp_not =
            dynamic_cast<StateSetNegation *>(formulas[sp_not_reg->get_stateset_id()].fpointer.get());
    if(!sp_not) {
        std::cerr << "Error when applying rule PR: set expression #" << left
                  << " is not the regression of a negation." << std::endl;
        return false;
    }
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(formulas[right].fpointer.get());
    if(!s_not) {
        std::cerr << "Error when applying rule PR: set expression #" << right
                  << " is not a negation." << std::endl;
        return false;
    }

    FormulaIndex si = s_not->get_child_id();
    FormulaIndex spi = sp_not->get_child_id();

    // check if premise says that S[A] \subseteq S'
    if(kbentries[premise]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule PR: knwoledge #" << premise
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(formulas[kbentries[premise]->get_first()].fpointer.get());
    if(!s_prog || s_prog->get_stateset_id() != si || kbentries[premise]->get_second() != spi) {
        std::cerr << "Error when applying rule PR: knowledge #" << premise
                  << " does not state that the progression of set expression #" << si
                  << " is a subset of set expression #" << spi << "." << std::endl;
        return false;
    }
    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left, right)), conclusion);
    return true;
}

// Regression to Progression: given (1) [A]S \subseteq S', then S'_not[A] \subseteq S_not
bool ProofChecker::check_rule_rp(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                                KnowledgeIndex premise) {
    assert(kbentries[premise] != nullptr);

    // check that left represents S'_not[A] and right S_not
    StateSetProgression *sp_not_prog =
            dynamic_cast<StateSetProgression *>(formulas[left].fpointer.get());
    if(!sp_not_prog) {
        std::cerr << "Error when applying rule RP: set expression #" << left
                  << " is not a progression." << std::endl;
        return false;
    }
    StateSetNegation *sp_not =
            dynamic_cast<StateSetNegation *>(formulas[sp_not_prog->get_stateset_id()].fpointer.get());
    if(!sp_not) {
        std::cerr << "Error when applying rule RP: set expression #" << left
                  << " is not the progression of a negation." << std::endl;
        return false;
    }
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(formulas[right].fpointer.get());
    if(!s_not) {
        std::cerr << "Error when applying rule RP: set expression #" << right
                  << " is not a negation." << std::endl;
        return false;
    }

    FormulaIndex si = s_not->get_child_id();
    FormulaIndex spi = sp_not->get_child_id();

    // check if premise says that [A]S \subseteq S'
    if(kbentries[premise]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule RP: knowledge #" << premise
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(formulas[kbentries[premise]->get_first()].fpointer.get());
    if(!s_reg || s_reg->get_stateset_id() != si || kbentries[premise]->get_second() != spi) {
        std::cerr << "Error when applying rule RP: knowledge #" << premise
                  << " does not state that the regression of set expression #" << si
                  << " is a subset of set expression #" << spi << "." << std::endl;
        return false;
    }
    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(left, right)), conclusion);
    return true;
}



/*
 * BASIC STATEMENTS
 */

// check if \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B1(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    try {
        std::vector<StateSetVariable *> left;
        std::vector<StateSetVariable *> right;

        StateSetVariable *reference_formula = gather_sets_intersection(formulas[fi1].fpointer.get(), left, right);
        if(!reference_formula) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(fi1)
                    + " is not a intersection of literals of the same type.";
            throw std::runtime_error(msg);
        }
        StateSetVariable *tmp = gather_sets_union(formulas[fi2].fpointer.get(), right, left);
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

        add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(fi1,fi2)), newki);
        ret = true;
    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
    if (g_discard_formulas) {
        remove_formulas_if_obsolete({fi1,fi2}, newki);
    }
    return ret;
}

// check if (\bigcap_{X \in \mathcal X} X)[A] \land \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B2(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    try {
        std::vector<StateSetVariable *> prog;
        std::vector<StateSetVariable *> left;
        std::vector<StateSetVariable *> right;
        std::unordered_set<int> actions;
        StateSet *prog_formula = nullptr;
        StateSet *left_formula = nullptr;

        SetFormulaType left_type = formulas[fi1].fpointer.get()->get_formula_type();
        /*
         * We expect the left side to either be a progression or an intersection with
         * a progression on the left side
         */
        if (left_type == SetFormulaType::INTERSECTION) {
            StateSetIntersection *intersection =
                    dynamic_cast<StateSetIntersection *>(formulas[fi1].fpointer.get());
            prog_formula = formulas[intersection->get_left_id()].fpointer.get();
            left_formula = formulas[intersection->get_right_id()].fpointer.get();
        } else if (left_type == SetFormulaType::PROGRESSION) {
            prog_formula = formulas[fi1].fpointer.get();
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
        prog_formula = formulas[progression->get_stateset_id()].fpointer.get();
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
        StateSetVariable *tmp = gather_sets_union(formulas[fi2].fpointer.get(), right, left);
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
        add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(fi1,fi2)), newki);
        ret = true;

    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }

    if (g_discard_formulas) {
        remove_formulas_if_obsolete({fi1,fi2}, newki);
    }
    return ret;
}

// check if [A](\bigcap_{X \in \mathcal X} X) \land \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B3(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    try {
        std::vector<StateSetVariable *> reg;
        std::vector<StateSetVariable *> left;
        std::vector<StateSetVariable *> right;
        std::unordered_set<int> actions;
        StateSet *reg_formula = nullptr;
        StateSet *left_formula = nullptr;

        SetFormulaType left_type = formulas[fi1].fpointer.get()->get_formula_type();
        /*
         * We expect the left side to either be a regression or an intersection with
         * a regression on the left side
         */
        if (left_type == SetFormulaType::INTERSECTION) {
            StateSetIntersection *intersection =
                    dynamic_cast<StateSetIntersection *>(formulas[fi1].fpointer.get());
            reg_formula = formulas[intersection->get_left_id()].fpointer.get();
            left_formula = formulas[intersection->get_right_id()].fpointer.get();
        } else if (left_type == SetFormulaType::REGRESSION) {
            reg_formula = formulas[fi1].fpointer.get();
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
        reg_formula = formulas[regression->get_stateset_id()].fpointer.get();
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
        StateSetVariable *tmp = gather_sets_union(formulas[fi2].fpointer.get(), right, left);
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
        add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(fi1,fi2)), newki);
        ret = true;

    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }

    if (g_discard_formulas) {
        remove_formulas_if_obsolete({fi1,fi2}, newki);
    }
    return ret;
}


// check if L \subseteq L', where L and L' might be represented by different formalisms
bool ProofChecker::check_statement_B4(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = true;

    try {
        StateSet *left = formulas[fi1].fpointer.get();
        bool left_positive = true;
        StateSet *right = formulas[fi2].fpointer.get();
        bool right_positive = true;

        if(left->get_formula_type() == SetFormulaType::NEGATION) {
            StateSetNegation *f1neg = dynamic_cast<StateSetNegation *>(left);
            left = formulas[f1neg->get_child_id()].fpointer.get();
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
            right = formulas[f2neg->get_child_id()].fpointer.get();
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

        add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(fi1,fi2)), newki);
    } catch (std::runtime_error e) {
        std::cerr << e.what();
        ret = false;
    }

    if (g_discard_formulas) {
        remove_formulas_if_obsolete({fi1,fi2}, newki);
    }
    return ret;
}


// check if A \subseteq A'
bool ProofChecker::check_statement_B5(KnowledgeIndex newki,
                                      ActionSetIndex ai1,
                                      ActionSetIndex ai2) {
    if(!actionsets[ai1].get()->is_subset(actionsets[ai2].get())) {
        std::cerr << "Error when checking statement B5: action set #"
                  << ai1 << " is not a subset of action set #" << ai2 << "." << std::endl;
        return false;
    } else {
        add_kbentry(std::unique_ptr<KBEntry>(new KBEntryAction(ai1,ai2)), newki);
        return true;
    }
}


bool ProofChecker::is_unsolvability_proven() {
    return unsolvability_proven;
}
