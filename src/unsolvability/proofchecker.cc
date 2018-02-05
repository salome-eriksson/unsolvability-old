#include "proofchecker.h"

#include <cassert>
#include <stack>
#include <fstream>
#include <math.h>
#include <limits>

#include "setformulaconstant.h"
#include "setformulacompound.h"

// TODO: should all error messages here be printed in cerr?

KBEntry::KBEntry() {

}

ProofChecker::ProofChecker()
    : unsolvability_proven(false) {

}

void ProofChecker::add_kbentry(KBEntry *entry, KnowledgeIndex index) {
    assert(index >= kbentries.size());
    if(index > kbentries.size()) {
        kbentries.resize(index);
    }
    kbentries.push_back(entry);
}

void ProofChecker::add_formula(SetFormula *formula, FormulaIndex index) {
    // first_pass() will guarantee that the entry for this index exists already
    assert(formulas[index].fpointer == nullptr);
    formulas[index].fpointer = formula;
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
                formulas.resize(mainsetid+1, FormulaEntry(nullptr, -1));
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

    SetFormulaConstant *f = dynamic_cast<SetFormulaConstant *>(formulas[fi].fpointer);
    if ((!f) || (f->get_constant_type() != ConstantType::EMPTY)) {
        std::cerr << "Error when applying rule D1: set expression #" << fi
                  << " is not the constant empty set." << std::endl;
        return false;
    }
    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that f=S \union S' is dead based on k1 (S is dead) and k2 (S' is dead)
bool ProofChecker::check_rule_D2(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2) {
    assert(kbentries[ki1] != nullptr && kbentries[ki2] != nullptr);

    // f represents left \cup right
    SetFormulaUnion *f = dynamic_cast<SetFormulaUnion *>(formulas[fi].fpointer);
    if (!f) {
        std::cerr << "Error when applying rule D2: set expression #" << fi
                  << "is not a union." << std::endl;
        return false;
    }
    FormulaIndex lefti = f->get_left_index();
    FormulaIndex righti = f->get_right_index();

    // check if k1 says that left is dead
    if ((kbentries[ki1]->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[ki1]->get_first() != lefti)) {
        std::cerr << "Error when applying rule D2: Knowledge #" << ki1
                  << "does not state that set expression #" << lefti
                  << " is dead." << std::endl;
        return false;
    }

    // check if k2 says that right is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[ki2]->get_first() != righti)) {
        std::cerr << "Error when applying rule D2: Knowledge #" << ki2
                  << "does not state that set expression #" << righti
                  << " is dead." << std::endl;
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
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
    add_kbentry(new KBEntryDead(fi), newki);
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
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki]->get_first()].fpointer);
    if ((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule D4: knowledge #" << ki
                  << " does not state that the constant initial set is dead." << std::endl;
        return false;
    }

    add_kbentry(new KBEntryUnsolvable(), newki);
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
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki]->get_first()].fpointer);
    if ( (!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule D5: knowledge #" << ki
                  << " does not state that the constant goal set is dead." << std::endl;
        return false;
    }

    add_kbentry(new KBEntryUnsolvable(), newki);
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
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki1]->get_first()].fpointer);
    if ((!s_prog) || (s_prog->get_subformula_index() != fi)) {
        std::cerr << "Error when applying rule D6: the left side of subset knowledge #" << ki1
                  << " is not the progression of set expression #" << fi << "." << std::endl;
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()].fpointer);
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
            dynamic_cast<SetFormulaIntersection *>(formulas[kbentries[ki3]->get_first()].fpointer);
    // check if left side of s_and goal is S
    if ((!s_and_goal) || (s_and_goal->get_left_index() != fi)) {
        std::cerr << "Error when applying rule D6: the set expression declared dead in knowledge #"
                  << ki3 << " is not an intersection with set expression #" << fi
                  << " on the left side." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_and_goal->get_right_index()].fpointer);
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule D6: the set expression declared dead in knowledge #"
                  << ki3 << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that S_not is dead based on k1 (S[A] \subseteq S \cup S'),
// k2 (S' is dead) and k3 ({I} \subseteq S_not)
bool ProofChecker::check_rule_D7(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {
    assert(kbentries[ki1] != nullptr && kbentries[ki2] != nullptr && kbentries[ki3] != nullptr);

    // check if fi corresponds to s_not
    SetFormulaNegation *s_not = dynamic_cast<SetFormulaNegation *>(formulas[fi].fpointer);
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
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki1]->get_first()].fpointer);
    if ((!s_prog) || (s_prog->get_subformula_index() != si)) {
        std::cerr << "Error when applying rule D7: the left side of subset knowledge #" << ki1
                  << " is not the progression of set expression #" << si << "." << std::endl;
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()].fpointer);
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
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki3]->get_first()].fpointer);
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

    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that S_not is dead based on k1 ([A]S \subseteq S \cup S'),
// k2 (S' is dead) and k3  (S_not \cap S_G(\Pi) is dead)
bool ProofChecker::check_rule_D8(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {
    assert(kbentries[ki1] != nullptr && kbentries[ki2] != nullptr && kbentries[ki3] != nullptr);

    // check if fi corresponds to s_not
    SetFormulaNegation *s_not = dynamic_cast<SetFormulaNegation *>(formulas[fi].fpointer);
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
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki1]->get_first()].fpointer);
    if ((!s_reg) || (s_reg->get_subformula_index() != si)) {
        std::cerr << "Error when applying rule D8: the left side of subset knowledge #" << ki1
                  << " is not the regression of set expression #" << si << "." << std::endl;
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()].fpointer);
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
            dynamic_cast<SetFormulaIntersection *>(formulas[kbentries[ki3]->get_first()].fpointer);
    // check if left side of s_not_and goal is S_not
    if ((!s_not_and_goal) || (s_not_and_goal->get_left_index() != fi)) {
        std::cerr << "Error when applying rule D8: the set expression declared dead in knowledge #"
                  << ki3 << " is not an intersection with set expression #" << fi
                  << " on the left side." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_not_and_goal->get_right_index()].fpointer);
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule D8: the set expression declared dead in knowledge #"
                  << ki3 << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
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
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki1]->get_first()].fpointer);
    if ((!s_reg) || (s_reg->get_subformula_index() != fi)) {
        std::cerr << "Error when applying rule D9: the left side of subset knowledge #" << ki1
                  << " is not the regression of set expression #" << fi << "." << std::endl;
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()].fpointer);
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
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki3]->get_first()].fpointer);
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule D9: the left side of subset knowledge #" << ki3
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of k3 is S_not
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[kbentries[ki3]->get_second()].fpointer);
    if((!s_not) || s_not->get_subformula_index() != fi) {
        std::cerr << "Error when applying rule D9: the right side of subset knowledge #" << ki3
                  << " is not the negation of set expression #" << fi << "." << std::endl;
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that S'_not[A] \subseteq S_not based on k ([A]S \subseteq S')
bool ProofChecker::check_rule_D10(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                                KnowledgeIndex ki) {
    assert(kbentries[ki] != nullptr);

    // check that f1 represents S'_not[A] and f2 S_not
    SetFormulaProgression *sp_not_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[fi1].fpointer);
    if(!sp_not_prog) {
        std::cerr << "Error when applying rule D10: set expression #" << fi1
                  << " is not a progression." << std::endl;
        return false;
    }
    SetFormulaNegation *sp_not =
            dynamic_cast<SetFormulaNegation *>(formulas[sp_not_prog->get_subformula_index()].fpointer);
    if(!sp_not) {
        std::cerr << "Error when applying rule D10: set expression #" << fi1
                  << " is not the progression of a negation." << std::endl;
        return false;
    }
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[fi2].fpointer);
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
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki]->get_first()].fpointer);
    if(!s_reg || s_reg->get_subformula_index() != si || kbentries[ki]->get_second() != spi) {
        std::cerr << "Error when applying rule D10: knowledge #" << ki
                  << " does not state that the regression of set expression #" << si
                  << " is a subset of set expression #" << spi << "." << std::endl;
        return false;
    }
    add_kbentry(new KBEntrySubset(fi1, fi2), newki);
    return true;
}


// KBEntry newki says that [A]S'_not \subseteq S_not based on k (S[A] \subseteq S')
bool ProofChecker::check_rule_D11(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                                KnowledgeIndex ki) {
    assert(kbentries[ki] != nullptr);

    //check that f1 represents [A]S'_not and f_2 S_not
    SetFormulaRegression *sp_not_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[fi1].fpointer);
    if(!sp_not_reg) {
        std::cerr << "Error when applying rule D11: set expression #" << fi1
                  << " is not a regression." << std::endl;
        return false;
    }
    SetFormulaNegation *sp_not =
            dynamic_cast<SetFormulaNegation *>(formulas[sp_not_reg->get_subformula_index()].fpointer);
    if(!sp_not) {
        std::cerr << "Error when applying rule D11: set expression #" << fi1
                  << " is not the regression of a negation." << std::endl;
        return false;
    }
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[fi2].fpointer);
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
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki]->get_first()].fpointer);
    if(!s_prog || s_prog->get_subformula_index() != si || kbentries[ki]->get_second() != spi) {
        std::cerr << "Error when applying rule D11: knowledge #" << ki
                  << " does not state that the progression of set expression #" << si
                  << " is a subset of set expression #" << spi << "." << std::endl;
        return false;
    }
    add_kbentry(new KBEntrySubset(fi1, fi2), newki);
    return true;
}


// check if L \subseteq L'
bool ProofChecker::check_statement_B1(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    FormulaIndex l_resolved = fi1;
    FormulaIndex lp_resolved = fi2;

    // resolve negated formulas and instead pass bools if the formulas are negated
    bool left_negated = false;
    bool right_negated = false;
    SetFormulaNegation *neg = dynamic_cast<SetFormulaNegation *>(formulas[fi1].fpointer);
    if(neg) {
        l_resolved = neg->get_subformula_index();
        left_negated = true;
    }
    neg = dynamic_cast<SetFormulaNegation *>(formulas[fi2].fpointer);
    if(neg) {
        lp_resolved = neg->get_subformula_index();
        right_negated = true;
    }

    if(formulas[l_resolved].fpointer->is_subset(formulas[lp_resolved].fpointer, left_negated, right_negated)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        ret = true;
    } else {
        std::cerr << "Error when checking statement B1: set expression #" << fi1
                  << " is not a subset of set expression #" << fi2 << "." << std::endl;
    }

    // delete formulas that are not needed anymore
    for(int index : {l_resolved, lp_resolved}) {
        if(formulas[index].last_occ == newki) {
            delete formulas[index].fpointer;
            formulas[index].fpointer = nullptr;
        }
    }
    return ret;
}

// check if X \subseteq X' \cup X''
bool ProofChecker::check_statement_B2(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    // check if fi2 represents X' \cup X''
    SetFormulaUnion *xp_cup_xpp = dynamic_cast<SetFormulaUnion *>(formulas[fi2].fpointer);
    if(!xp_cup_xpp) {
        std::cerr << "Error when checking statement B2: set expression #" << fi2
                  << " is not a union." << std::endl;
        return false;
    }
    FormulaIndex xpi = xp_cup_xpp->get_left_index();
    FormulaIndex xppi = xp_cup_xpp->get_right_index();

    if(formulas[fi1].fpointer->is_subset(formulas[xpi].fpointer, formulas[xppi].fpointer)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        ret = true;
    } else {
        std::cerr << "Error when checking statement B2: set expression #" << fi1
                  << " is not a subset of set expression #" << fi2 << "." << std::endl;
    }

    // delete formulas that are not needed anymore
    for(int index : {fi1, xpi, xppi}) {
        if(formulas[index].last_occ == newki) {
            delete formulas[index].fpointer;
            formulas[index].fpointer = nullptr;
        }
    }
    return ret;
}

// check if L \cap S_G(\Pi) \subseteq L'
bool ProofChecker::check_statement_B3(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    // check if fi1 represents L \cap S_G(\Pi)
    SetFormulaIntersection *l_cap_goal = dynamic_cast<SetFormulaIntersection *>(formulas[fi1].fpointer);
    if(!l_cap_goal) {
        std::cerr << "Error when checking statement B3: set expression #" << fi1
                  << " is not an intersection." << std::endl;
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[l_cap_goal->get_right_index()].fpointer);
    if(!goal) {
        std::cerr << "Error when checking statement B3: set expression #" << fi1
                  << " is not an intersection with the constant goal on the right side." << std::endl;
        return false;
    }

    FormulaIndex l_resolved = l_cap_goal->get_left_index();
    FormulaIndex lp_resolved = fi2;

    // resolve negated formulas and instead pass bools if the formulas are negated
    bool left_negated = false;
    bool right_negated = false;
    SetFormulaNegation *neg = dynamic_cast<SetFormulaNegation *>(formulas[l_resolved].fpointer);
    if(neg) {
        l_resolved = neg->get_subformula_index();
        left_negated = true;
    }
    neg = dynamic_cast<SetFormulaNegation *>(formulas[fi2].fpointer);
    if(neg) {
        lp_resolved = neg->get_subformula_index();
        right_negated = true;
    }

    if(formulas[l_resolved].fpointer->intersection_with_goal_is_subset(formulas[lp_resolved].fpointer, left_negated, right_negated)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        ret = true;
    } else {
        std::cerr << "Error when checking statement B3: set expression #" << fi1
                  << " is not a subset of set expression #" << fi2 << "." << std::endl;
    }

    // delete formulas that are not needed anymore
    for(int index : {l_resolved, lp_resolved}) {
        if(formulas[index].last_occ == newki) {
            delete formulas[index].fpointer;
            formulas[index].fpointer = nullptr;
        }
    }
    return ret;
}


// check if X[A] \subseteq X \cup L
bool ProofChecker::check_statement_B4(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    // check if fi1 represents X[A]
    SetFormulaProgression *x_prog = dynamic_cast<SetFormulaProgression *>(formulas[fi1].fpointer);
    if(!x_prog) {
        std::cerr << "Error when checking statement B4: set expression #" << fi1
                  << " is not a progression." << std::endl;
        return false;
    }
    FormulaIndex xi = x_prog->get_subformula_index();

    //check if fi2 represents X \cup L
    SetFormulaUnion *x_cup_l = dynamic_cast<SetFormulaUnion *>(formulas[fi2].fpointer);
    if((!x_cup_l) || (x_cup_l->get_left_index() != xi)) {
        std::cerr << "Error when checking statemnt B4: set expression #" << fi2
                  << " is not a union with set expression #" << xi << " on the left side." << std::endl;
        return false;
    }
    FormulaIndex l_resolved = x_cup_l->get_right_index();

    //resolve negated formula and instead pass bool if the formula is negated
    bool negated = false;
    SetFormulaNegation *neg = dynamic_cast<SetFormulaNegation *>(formulas[l_resolved].fpointer);
    if(neg) {
        l_resolved = neg->get_subformula_index();
        negated = true;
    }

    if(formulas[xi].fpointer->progression_is_union_subset(formulas[l_resolved].fpointer, negated)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        ret = true;
    } else {

        std::cerr << "Error when checking statement B4: set expression #" << fi1
                  << " is not a subset of set expression #" << fi2 << "." << std::endl;    }

    // delete formulas that are not needed anymore
    for(int index : {xi, l_resolved}) {
        if(formulas[index].last_occ == newki) {
            delete formulas[index].fpointer;
            formulas[index].fpointer = nullptr;
        }
    }
    return ret;
}


// check if [A]X \subseteq X \cup L
bool ProofChecker::check_statement_B5(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {
    bool ret = false;

    // check if fi1 represents [A]X
    SetFormulaRegression *x_reg = dynamic_cast<SetFormulaRegression *>(formulas[fi1].fpointer);
    if(!x_reg) {
        std::cerr << "Error when checking statement B5: set expression #" << fi1
                  << " is not a regression." << std::endl;
        return false;
    }
    FormulaIndex xi = x_reg->get_subformula_index();

    //check if fi2 represents X \cup L
    SetFormulaUnion *x_cup_l = dynamic_cast<SetFormulaUnion *>(formulas[fi2].fpointer);
    if((!x_cup_l) || (x_cup_l->get_left_index() != xi)) {
        std::cerr << "Error when checking statement B5: set expression #" << fi2
                  << " is not a union with set expression #" << xi << " on the left side." << std::endl;
        return false;
    }
    FormulaIndex l_resolved = x_cup_l->get_right_index();

    //resolve negated formula and instead pass bool if the formula is negated
    bool negated = false;
    SetFormulaNegation *neg = dynamic_cast<SetFormulaNegation *>(formulas[l_resolved].fpointer);
    if(neg) {
        l_resolved = neg->get_subformula_index();
        negated = true;
    }

    if(formulas[xi].fpointer->regression_is_union_subset(formulas[l_resolved].fpointer, negated)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        ret = true;
    } else {
        std::cerr << "Error when checking statement B5: set expression #" << fi1
                  << " is not a subset of set expression #" << fi2 << "." << std::endl;
    }

    // delete formulas that are not needed anymore
    for(int index : {xi, l_resolved}) {
        if(formulas[index].last_occ == newki) {
            delete formulas[index].fpointer;
            formulas[index].fpointer = nullptr;
        }
    }
    return ret;
}


bool ProofChecker::is_unsolvability_proven() {
    return unsolvability_proven;
}
