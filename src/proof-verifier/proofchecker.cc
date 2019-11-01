#include "proofchecker.h"

#include <cassert>
#include <stack>
#include <fstream>
#include <math.h>
#include <limits>

#include "global_funcs.h"
#include "setformulaconstant.h"
#include "setformulacompound.h"

// TODO: should all error messages here be printed in cerr?

KBEntry::KBEntry() {

}

ProofChecker::ProofChecker()
    : unsolvability_proven(false) {

}

void ProofChecker::add_kbentry(std::unique_ptr<KBEntry> entry, KnowledgeIndex index) {
    assert(index >= kbentries.size());
    if(index > kbentries.size()) {
        kbentries.resize(index);
    }
    kbentries.push_back(std::move(entry));
}

SetFormula *ProofChecker::gather_sets_intersection(SetFormula *f,
                                            std::vector<SetFormula *> &positive,
                                            std::vector<SetFormula *> &negative) {
    SetFormula *ret = nullptr;
    switch(f->get_formula_type()) {
    case SetFormulaType::CONSTANT:
    case SetFormulaType::BDD:
    case SetFormulaType::HORN:
    case SetFormulaType::TWOCNF:
    case SetFormulaType::EXPLICIT:
        positive.push_back(f);
        ret = f;
        break;
    case SetFormulaType::NEGATION:
        f = formulas[dynamic_cast<SetFormulaNegation *>(f)->get_subformula_index()].fpointer.get();
        ret = gather_sets_intersection(f, negative, positive);
        break;
    case SetFormulaType::INTERSECTION: {
        SetFormulaIntersection *fi = dynamic_cast<SetFormulaIntersection *>(f);
        ret = gather_sets_intersection(formulas[fi->get_left_index()].fpointer.get(),
                positive, negative);
        if(ret) {
            SetFormula *ret2 = gather_sets_intersection(formulas[fi->get_right_index()].fpointer.get(),
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

SetFormula *ProofChecker::gather_sets_union(SetFormula *f,
                                     std::vector<SetFormula *> &positive,
                                     std::vector<SetFormula *> &negative) {
    SetFormula *ret = nullptr;
    switch(f->get_formula_type()) {
    case SetFormulaType::CONSTANT:
    case SetFormulaType::BDD:
    case SetFormulaType::HORN:
    case SetFormulaType::TWOCNF:
    case SetFormulaType::EXPLICIT:
        positive.push_back(f);
        ret = f;
        break;
    case SetFormulaType::NEGATION:
        f = formulas[dynamic_cast<SetFormulaNegation *>(f)->get_subformula_index()].fpointer.get();
        ret = gather_sets_union(f, negative, positive);
        break;
    case SetFormulaType::UNION: {
        SetFormulaUnion *fi = dynamic_cast<SetFormulaUnion *>(f);
        ret = gather_sets_union(formulas[fi->get_left_index()].fpointer.get(),
                positive, negative);
        if(ret) {
            SetFormula *ret2 = gather_sets_union(formulas[fi->get_right_index()].fpointer.get(),
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

SetFormula *ProofChecker::update_reference_and_check_consistency(
        SetFormula *reference_formula, SetFormula *tmp, std::string stmt) {
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

void ProofChecker::add_formula(std::unique_ptr<SetFormula> formula, FormulaIndex index) {
    // if g_discard_formulas, first_pass() will guarantee that the entry for this index exists already
    if (!g_discard_formulas && index >= formulas.size()) {
        formulas.resize(index+1);
    }
    assert(!formulas[index].fpointer);
    formulas[index].fpointer = std::move(formula);
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
                SetFormulaNegation *f = dynamic_cast<SetFormulaNegation *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_subformula_index()}, current_ki);
                break;
            }
            case SetFormulaType::PROGRESSION: {
                SetFormulaProgression *f = dynamic_cast<SetFormulaProgression *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_subformula_index()}, current_ki);
                break;
            }
            case SetFormulaType::REGRESSION: {
                SetFormulaRegression *f = dynamic_cast<SetFormulaRegression *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_subformula_index()}, current_ki);
                break;
            }
            case SetFormulaType::INTERSECTION: {
                SetFormulaIntersection *f = dynamic_cast<SetFormulaIntersection *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_left_index(), f->get_right_index()}, current_ki);
                break;
            }
            case SetFormulaType::UNION: {
                SetFormulaUnion *f = dynamic_cast<SetFormulaUnion *>(formulas[index].fpointer.get());
                remove_formulas_if_obsolete({f->get_left_index(), f->get_right_index()}, current_ki);
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

// KBEntry newki says that f=emptyset is dead
bool ProofChecker::check_rule_D1(KnowledgeIndex newki, FormulaIndex fi) {

    SetFormulaConstant *f = dynamic_cast<SetFormulaConstant *>(formulas[fi].fpointer.get());
    if ((!f) || (f->get_constant_type() != ConstantType::EMPTY)) {
        std::cerr << "Error when applying rule D1: set expression #" << fi
                  << " is not the constant empty set." << std::endl;
        return false;
    }
    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(fi)), newki);
    return true;
}


// KBEntry newki says that f=S \union S' is dead based on k1 (S is dead) and k2 (S' is dead)
bool ProofChecker::check_rule_D2(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2) {
    assert(kbentries[ki1] && kbentries[ki2]);

    // f represents left \cup right
    SetFormulaUnion *f = dynamic_cast<SetFormulaUnion *>(formulas[fi].fpointer.get());
    if (!f) {
        std::cerr << "Error when applying rule D2: set expression #" << fi
                  << "is not a union." << std::endl;
        return false;
    }
    FormulaIndex lefti = f->get_left_index();
    FormulaIndex righti = f->get_right_index();

    // check if k1 says that left is dead
    if ((kbentries[ki1].get()->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[ki1].get()->get_first() != lefti)) {
        std::cerr << "Error when applying rule D2: Knowledge #" << ki1
                  << "does not state that set expression #" << lefti
                  << " is dead." << std::endl;
        return false;
    }

    // check if k2 says that right is dead
    if ((kbentries[ki2].get()->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[ki2].get()->get_first() != righti)) {
        std::cerr << "Error when applying rule D2: Knowledge #" << ki2
                  << "does not state that set expression #" << righti
                  << " is dead." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(fi)), newki);
    return true;
}


// KBEntry newki says that f=S is dead based on k1 (S \subseteq S') and k2 (S' is dead)
bool ProofChecker::check_rule_D3(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2) {
    assert(kbentries[ki1] != nullptr && kbentries[ki2] != nullptr);

    // check if k1 says that f is a subset of "x" (x can be anything)
    if ((kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) ||
       (kbentries[ki1]->get_first() != fi)) {
        std::cerr << "Error when applying rule D3: knowledge #" << ki1
                  << " does not state that set expression #" << fi
                  << " is a subset of another set." << std::endl;
        return false;
    }

    FormulaIndex xi = kbentries[ki1]->get_second();

    // check if k2 says that x is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != xi)) {
        std::cerr << "Error when applying rule D3: knowledge #" << ki1
                  << " states that set expression #" << fi
                  << " is a subset of set expression #" << xi << ", but knowledge #" << ki2
                  << " does not state that " << xi << " is dead." << std::endl;
        return false;
    }
    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(fi)), newki);
    return true;
}


// KBEntry newki says that the task is unsolvable based on k ({I} is dead)
bool ProofChecker::check_rule_D4(KnowledgeIndex newki, KnowledgeIndex ki) {
    assert(kbentries[ki] != nullptr);

    // check that k says that {I} is dead
    if (kbentries[ki]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule D4: knowledge #" << ki
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki]->get_first()].fpointer.get());
    if ((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule D4: knowledge #" << ki
                  << " does not state that the constant initial set is dead." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryUnsolvable()), newki);
    unsolvability_proven = true;
    return true;
}

// KBEntry newki says that the task is unsolvable based on k (S_G(\Pi) is dead)
bool ProofChecker::check_rule_D5(KnowledgeIndex newki, KnowledgeIndex ki) {
    assert(kbentries[ki] != nullptr);

    // check that k says that S_G(\Pi) is dead
    if (kbentries[ki]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule D5: knowledge #" << ki
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki]->get_first()].fpointer.get());
    if ( (!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule D5: knowledge #" << ki
                  << " does not state that the constant goal set is dead." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryUnsolvable()), newki);
    unsolvability_proven = true;
    return true;
}


// KBEntry newki says that S is dead based on k1 (S[A] \subseteq S \cup S'),
// k2 (S' is dead) and k3 (S \cap S_G(\Pi) is dead)
bool ProofChecker::check_rule_D6(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {
    assert(kbentries[ki1] != nullptr && kbentries[ki2] != nullptr && kbentries[ki3] != nullptr);

    // check if k1 says that S[A] \subseteq S \cup S'
    if (kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule D6: knowledge #" << ki1
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of k1 is S[A]
    SetFormulaProgression *s_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki1]->get_first()].fpointer.get());
    if ((!s_prog) || (s_prog->get_subformula_index() != fi)) {
        std::cerr << "Error when applying rule D6: the left side of subset knowledge #" << ki1
                  << " is not the progression of set expression #" << fi << "." << std::endl;
        return false;
    }
    if(!actionsets[s_prog->get_actionset_index()].get()->is_constantall()) {
        std::cerr << "Error when applying rule D6: "
                     "the progression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()].fpointer.get());
    if((!s_cup_sp) || (s_cup_sp->get_left_index() != fi)) {
        std::cerr << "Error when applying rule D6: the right side of subset knowledge #" << ki1
                  << " is not a union of set expression #" << fi
                  << " and another set expression." << std::endl;
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_index();

    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != spi)) {
        std::cerr << "Error when applying rule D6: knowledge #" << ki2
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if k3 says that S \cap S_G(\Pi) is dead
    if (kbentries[ki3]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule D6: knowledge #" << ki3
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    SetFormulaIntersection *s_and_goal =
            dynamic_cast<SetFormulaIntersection *>(formulas[kbentries[ki3]->get_first()].fpointer.get());
    // check if left side of s_and goal is S
    if ((!s_and_goal) || (s_and_goal->get_left_index() != fi)) {
        std::cerr << "Error when applying rule D6: the set expression declared dead in knowledge #"
                  << ki3 << " is not an intersection with set expression #" << fi
                  << " on the left side." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_and_goal->get_right_index()].fpointer.get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule D6: the set expression declared dead in knowledge #"
                  << ki3 << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(fi)), newki);
    return true;
}


// KBEntry newki says that S_not is dead based on k1 (S[A] \subseteq S \cup S'),
// k2 (S' is dead) and k3 ({I} \subseteq S_not)
bool ProofChecker::check_rule_D7(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {
    assert(kbentries[ki1] != nullptr && kbentries[ki2] != nullptr && kbentries[ki3] != nullptr);

    // check if fi corresponds to s_not
    SetFormulaNegation *s_not = dynamic_cast<SetFormulaNegation *>(formulas[fi].fpointer.get());
    if(!s_not) {
        std::cerr << "Error when applying rule D7: set expression #" << fi
                  << " is not a negation." << std::endl;
        return false;
    }
    FormulaIndex si = s_not->get_subformula_index();

    // check if k1 says that S[A] \subseteq S \cup S'
    if (kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule D7: knowledge #" << ki1
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of k1 is S[A]
    SetFormulaProgression *s_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki1]->get_first()].fpointer.get());
    if ((!s_prog) || (s_prog->get_subformula_index() != si)) {
        std::cerr << "Error when applying rule D7: the left side of subset knowledge #" << ki1
                  << " is not the progression of set expression #" << si << "." << std::endl;
        return false;
    }
    if(!actionsets[s_prog->get_actionset_index()].get()->is_constantall()) {
        std::cerr << "Error when applying rule D7: "
                     "the progression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()].fpointer.get());
    if((!s_cup_sp) || (s_cup_sp->get_left_index() != si)) {
        std::cerr << "Error when applying rule D7: the right side of subset knowledge #" << ki1
                  << " is not a union of set expression #" << si
                  << " and another set expression." << std::endl;
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_index();

    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != spi)) {
        std::cerr << "Error when applying rule D7: knowledge #" << ki2
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if k3 says that {I} \subseteq S
    if (kbentries[ki3]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule D7: knowledge #" << ki3
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of k3 is {I}
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki3]->get_first()].fpointer.get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule D7: the left side of subset knowledge #" << ki3
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of k3 is S
    if(kbentries[ki3]->get_second() != si) {
        std::cerr << "Error when applying rule D7: the right side of subset knowledge #" << ki3
                  << " is not set expression #" << si << "." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(fi)), newki);
    return true;
}


// KBEntry newki says that S_not is dead based on k1 ([A]S \subseteq S \cup S'),
// k2 (S' is dead) and k3  (S_not \cap S_G(\Pi) is dead)
bool ProofChecker::check_rule_D8(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {
    assert(kbentries[ki1] != nullptr && kbentries[ki2] != nullptr && kbentries[ki3] != nullptr);

    // check if fi corresponds to s_not
    SetFormulaNegation *s_not = dynamic_cast<SetFormulaNegation *>(formulas[fi].fpointer.get());
    if(!s_not) {
        std::cerr << "Error when applying rule D8: set expression #" << fi
                  << " is not a negation." << std::endl;
        return false;
    }
    FormulaIndex si = s_not->get_subformula_index();

    // check if k1 says that [A]S \subseteq S \cup S'
    if(kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule D8: knowledge #" << ki1
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of k1 is [A]S
    SetFormulaRegression *s_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki1]->get_first()].fpointer.get());
    if ((!s_reg) || (s_reg->get_subformula_index() != si)) {
        std::cerr << "Error when applying rule D8: the left side of subset knowledge #" << ki1
                  << " is not the regression of set expression #" << si << "." << std::endl;
        return false;
    }
    if(!actionsets[s_reg->get_actionset_index()].get()->is_constantall()) {
        std::cerr << "Error when applying rule D8: "
                     "the regression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()].fpointer.get());
    if((!s_cup_sp) || (s_cup_sp->get_left_index() != si)) {
        std::cerr << "Error when applying rule D8: the right side of subset knowledge #" << ki1
                  << " is not a union of set expression #" << si
                  << " and another set expression." << std::endl;
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_index();

    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != spi)) {
        std::cerr << "Error when applying rule D8: knowledge #" << ki2
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if k3 says that S_not \cap S_G(\Pi) is dead
    if (kbentries[ki3]->get_kbentry_type() != KBType::DEAD) {
        std::cerr << "Error when applying rule D8: knowledge #" << ki3
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    SetFormulaIntersection *s_not_and_goal =
            dynamic_cast<SetFormulaIntersection *>(formulas[kbentries[ki3]->get_first()].fpointer.get());
    // check if left side of s_not_and goal is S_not
    if ((!s_not_and_goal) || (s_not_and_goal->get_left_index() != fi)) {
        std::cerr << "Error when applying rule D8: the set expression declared dead in knowledge #"
                  << ki3 << " is not an intersection with set expression #" << fi
                  << " on the left side." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_not_and_goal->get_right_index()].fpointer.get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule D8: the set expression declared dead in knowledge #"
                  << ki3 << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(fi)), newki);
    return true;
}


// KBEntry newki says that S is dead based on k1 ([A]S \subseteq S \cup S'),
// k2 (S' is dead) and k3 ({I} \subseteq S_not)
bool ProofChecker::check_rule_D9(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {
    assert(kbentries[ki1] != nullptr && kbentries[ki2] != nullptr && kbentries[ki3] != nullptr);

    // check if k1 says that [A]S \subseteq S \cup S'
    if(kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule D9: knowledge #" << ki1
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of k1 is [A]S
    SetFormulaRegression *s_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki1]->get_first()].fpointer.get());
    if ((!s_reg) || (s_reg->get_subformula_index() != fi)) {
        std::cerr << "Error when applying rule D9: the left side of subset knowledge #" << ki1
                  << " is not the regression of set expression #" << fi << "." << std::endl;
        return false;
    }
    if(!actionsets[s_reg->get_actionset_index()].get()->is_constantall()) {
        std::cerr << "Error when applying rule D9: "
                     "the regression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()].fpointer.get());
    if((!s_cup_sp) || (s_cup_sp->get_left_index() != fi)) {
        std::cerr << "Error when applying rule D9: the right side of subset knowledge #" << ki1
                  << " is not a union of set expression #" << fi
                  << " and another set expression." << std::endl;
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_index();

    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != spi)) {
        std::cerr << "Error when applying rule D9: knowledge #" << ki2
                  << " does not state that set expression #" << spi << " is dead." << std::endl;
        return false;
    }

    // check if k3 says that {I} \subseteq S_not
    if (kbentries[ki3]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule D9: knowledge #" << ki3
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of k3 is {I}
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki3]->get_first()].fpointer.get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule D9: the left side of subset knowledge #" << ki3
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of k3 is S_not
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[kbentries[ki3]->get_second()].fpointer.get());
    if((!s_not) || s_not->get_subformula_index() != fi) {
        std::cerr << "Error when applying rule D9: the right side of subset knowledge #" << ki3
                  << " is not the negation of set expression #" << fi << "." << std::endl;
        return false;
    }

    add_kbentry(std::unique_ptr<KBEntry>(new KBEntryDead(fi)), newki);
    return true;
}


// KBEntry newki says that S'_not[A] \subseteq S_not based on k ([A]S \subseteq S')
bool ProofChecker::check_rule_D10(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                                KnowledgeIndex ki) {
    assert(kbentries[ki] != nullptr);

    // check that f1 represents S'_not[A] and f2 S_not
    SetFormulaProgression *sp_not_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[fi1].fpointer.get());
    if(!sp_not_prog) {
        std::cerr << "Error when applying rule D10: set expression #" << fi1
                  << " is not a progression." << std::endl;
        return false;
    }
    SetFormulaNegation *sp_not =
            dynamic_cast<SetFormulaNegation *>(formulas[sp_not_prog->get_subformula_index()].fpointer.get());
    if(!sp_not) {
        std::cerr << "Error when applying rule D10: set expression #" << fi1
                  << " is not the progression of a negation." << std::endl;
        return false;
    }
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[fi2].fpointer.get());
    if(!s_not) {
        std::cerr << "Error when applying rule D10: set expression #" << fi2
                  << " is not a negation." << std::endl;
        return false;
    }

    FormulaIndex si = s_not->get_subformula_index();
    FormulaIndex spi = sp_not->get_subformula_index();

    // check if k says that [A]S \subseteq S'
    if(kbentries[ki]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule D10: knwoledge #" << ki
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    SetFormulaRegression *s_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki]->get_first()].fpointer.get());
    if(!s_reg || s_reg->get_subformula_index() != si || kbentries[ki]->get_second() != spi) {
        std::cerr << "Error when applying rule D10: knowledge #" << ki
                  << " does not state that the regression of set expression #" << si
                  << " is a subset of set expression #" << spi << "." << std::endl;
        return false;
    }
    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(fi1, fi2)), newki);
    return true;
}


// KBEntry newki says that [A]S'_not \subseteq S_not based on k (S[A] \subseteq S')
bool ProofChecker::check_rule_D11(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                                KnowledgeIndex ki) {
    assert(kbentries[ki] != nullptr);

    //check that f1 represents [A]S'_not and f_2 S_not
    SetFormulaRegression *sp_not_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[fi1].fpointer.get());
    if(!sp_not_reg) {
        std::cerr << "Error when applying rule D11: set expression #" << fi1
                  << " is not a regression." << std::endl;
        return false;
    }
    SetFormulaNegation *sp_not =
            dynamic_cast<SetFormulaNegation *>(formulas[sp_not_reg->get_subformula_index()].fpointer.get());
    if(!sp_not) {
        std::cerr << "Error when applying rule D11: set expression #" << fi1
                  << " is not the regression of a negation." << std::endl;
        return false;
    }
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[fi2].fpointer.get());
    if(!s_not) {
        std::cerr << "Error when applying rule D11: set expression #" << fi2
                  << " is not a negation." << std::endl;
        return false;
    }

    FormulaIndex si = s_not->get_subformula_index();
    FormulaIndex spi = sp_not->get_subformula_index();

    // check if k says that S[A] \subseteq S'
    if(kbentries[ki]->get_kbentry_type() != KBType::SUBSET) {
        std::cerr << "Error when applying rule D11: knwoledge #" << ki
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    SetFormulaProgression *s_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki]->get_first()].fpointer.get());
    if(!s_prog || s_prog->get_subformula_index() != si || kbentries[ki]->get_second() != spi) {
        std::cerr << "Error when applying rule D11: knowledge #" << ki
                  << " does not state that the progression of set expression #" << si
                  << " is a subset of set expression #" << spi << "." << std::endl;
        return false;
    }
    add_kbentry(std::unique_ptr<KBEntry>(new KBEntrySubset(fi1, fi2)), newki);
    return true;
}


// check if \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B1(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    try {
        std::vector<SetFormula *> left;
        std::vector<SetFormula *> right;

        SetFormula *reference_formula = gather_sets_intersection(formulas[fi1].fpointer.get(), left, right);
        if(!reference_formula) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(fi1)
                    + " is not a intersection of literals of the same type.";
            throw std::runtime_error(msg);
        }
        SetFormula *tmp = gather_sets_union(formulas[fi2].fpointer.get(), right, left);
        if (!tmp) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(fi2)
                    + " is not a union of literals of the same type.";
            throw std::runtime_error(msg);
        }
        reference_formula =
                update_reference_and_check_consistency(reference_formula, tmp, "B1");

        if (!reference_formula->is_subset(left, right)) {
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
        std::vector<SetFormula *> prog;
        std::vector<SetFormula *> left;
        std::vector<SetFormula *> right;
        std::unordered_set<int> actions;
        SetFormula *prog_formula = nullptr;
        SetFormula *left_formula = nullptr;

        SetFormulaType left_type = formulas[fi1].fpointer.get()->get_formula_type();
        /*
         * We expect the left side to either be a progression or an intersection with
         * a progression on the left side
         */
        if (left_type == SetFormulaType::INTERSECTION) {
            SetFormulaIntersection *intersection =
                    dynamic_cast<SetFormulaIntersection *>(formulas[fi1].fpointer.get());
            prog_formula = formulas[intersection->get_left_index()].fpointer.get();
            left_formula = formulas[intersection->get_right_index()].fpointer.get();
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
        SetFormulaProgression *progression = dynamic_cast<SetFormulaProgression *>(prog_formula);
        actionsets[progression->get_actionset_index()]->get_actions(actions);
        prog_formula = formulas[progression->get_subformula_index()].fpointer.get();
        // here, prog_formula is S (without [A])

        /*
         * In the progression, we only allow set variables, not set literals.
         * We abuse left here and know it will stay empty if the progression does
         * not contain set literals.
         */
        SetFormula *reference_formula = gather_sets_intersection(prog_formula, prog, left);
        if(!reference_formula || !left.empty()) {
            std::string msg = "Error when checking statement B2: "
                              "the progression in set expression #"
                    + std::to_string(fi1)
                    + " is not an intersection of set variables.";
            throw std::runtime_error(msg);
        }

        // left_formula is empty if the left side contains only a progression
        if(left_formula) {
            SetFormula *tmp = gather_sets_intersection(left_formula, left, right);
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
        SetFormula *tmp = gather_sets_union(formulas[fi2].fpointer.get(), right, left);
        if(!tmp) {
            std::string msg = "Error when checking statement B2: set expression #"
                    + std::to_string(fi2)
                    + " is not a union of literals.";
            throw std::runtime_error(msg);
        }
        reference_formula =
                update_reference_and_check_consistency(reference_formula, tmp, "B2");

        if(!reference_formula->is_subset_with_progression(
                    left, right, prog, actions)) {
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
        std::vector<SetFormula *> reg;
        std::vector<SetFormula *> left;
        std::vector<SetFormula *> right;
        std::unordered_set<int> actions;
        SetFormula *reg_formula = nullptr;
        SetFormula *left_formula = nullptr;

        SetFormulaType left_type = formulas[fi1].fpointer.get()->get_formula_type();
        /*
         * We expect the left side to either be a regression or an intersection with
         * a regression on the left side
         */
        if (left_type == SetFormulaType::INTERSECTION) {
            SetFormulaIntersection *intersection =
                    dynamic_cast<SetFormulaIntersection *>(formulas[fi1].fpointer.get());
            reg_formula = formulas[intersection->get_left_index()].fpointer.get();
            left_formula = formulas[intersection->get_right_index()].fpointer.get();
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
        SetFormulaRegression *regression = dynamic_cast<SetFormulaRegression *>(reg_formula);
        actionsets[regression->get_actionset_index()]->get_actions(actions);
        reg_formula = formulas[regression->get_subformula_index()].fpointer.get();
        // here, reg_formula is S (without [A])

        /*
         * In the regression, we only allow set variables, not set literals.
         * We abuse left here and know it will stay empty if the regression does
         * not contain set literals.
         */
        SetFormula *reference_formula = gather_sets_intersection(reg_formula, reg, left);
        if(!reference_formula || !left.empty()) {
            std::string msg = "Error when checking statement B3: "
                              "the regression in set expression #"
                    + std::to_string(fi1)
                    + " is not an intersection of set variables.";
            throw std::runtime_error(msg);
        }

        // left_formula is empty if the left side contains only a progression
        if(left_formula) {
            SetFormula *tmp = gather_sets_intersection(left_formula, left, right);
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
        SetFormula *tmp = gather_sets_union(formulas[fi2].fpointer.get(), right, left);
        if(!tmp) {
            std::string msg = "Error when checking statement B3: set expression #"
                    + std::to_string(fi2)
                    + " is not a union of literals.";
            throw std::runtime_error(msg);
        }
        reference_formula =
                update_reference_and_check_consistency(reference_formula, tmp, "B2");

        if(!reference_formula->is_subset_with_regression(
                    left, right, reg, actions)) {
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
        SetFormula *left = formulas[fi1].fpointer.get();
        bool left_positive = true;
        SetFormula *right = formulas[fi2].fpointer.get();
        bool right_positive = true;

        if(left->get_formula_type() == SetFormulaType::NEGATION) {
            SetFormulaNegation *f1neg = dynamic_cast<SetFormulaNegation *>(left);
            left = formulas[f1neg->get_subformula_index()].fpointer.get();
            left_positive = false;
        }
        switch (left->get_formula_type()) {
        case SetFormulaType::NEGATION:
        case SetFormulaType::INTERSECTION:
        case SetFormulaType::UNION:
        case SetFormulaType::PROGRESSION:
        case SetFormulaType::REGRESSION:
            std::string msg = "Error when checking statement B4: set expression #"
                    + std::to_string(fi1) + " is not a set literal.";
            throw std::runtime_error(msg);
            break;
        }

        if(right->get_formula_type() == SetFormulaType::NEGATION) {
            SetFormulaNegation *f2neg = dynamic_cast<SetFormulaNegation *>(right);
            right = formulas[f2neg->get_subformula_index()].fpointer.get();
            right_positive = false;
        }
        switch (right->get_formula_type()) {
        case SetFormulaType::NEGATION:
        case SetFormulaType::INTERSECTION:
        case SetFormulaType::UNION:
        case SetFormulaType::PROGRESSION:
        case SetFormulaType::REGRESSION:
            std::string msg = "Error when checking statement B4: set expression #"
                    + std::to_string(fi2) + " is not a set literal.";
            throw std::runtime_error(msg);
            break;
        }

        if(!left->is_subset_of(right, left_positive, right_positive)) {
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
