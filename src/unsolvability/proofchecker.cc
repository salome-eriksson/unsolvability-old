#include "proofchecker.h"

#include <cassert>

#include "setformulaconstant.h"
#include "setformulacompound.h"

// TODO: when returning false, print a reason

KBEntry::KBEntry() {

}

ProofChecker::ProofChecker()
    : unsolvability_proven(false) {

}

void ProofChecker::add_formula(SetFormula *formula, FormulaIndex index) {
    assert(index >= formulas.size());
    if(index > formulas.size()) {
        formulas.resize(index);
    }
    formulas.push_back(formula);
}

void ProofChecker::add_kbentry(KBEntry *entry, KnowledgeIndex index) {
    assert(index >= kbentries.size());
    if(index > kbentries.size()) {
        kbentries.resize(index);
    }
    kbentries.push_back(entry);
}

// KBEntry newki says that f=emptyset is dead
bool ProofChecker::check_rule_D1(KnowledgeIndex newki, FormulaIndex fi) {

    SetFormulaConstant *f = dynamic_cast<SetFormulaConstant *>(formulas[fi]);
    if ((!f) || (f->get_constant_type() != ConstantType::EMPTY)) {
        return false;
    }
    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that f=S \union S' is dead based on k1 (S is dead) and k2 (S' is dead)
bool ProofChecker::check_rule_D2(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2) {

    // f represents left \cup right
    SetFormulaUnion *f = dynamic_cast<SetFormulaUnion *>(formulas[fi]);
    if (!f) {
        return false;
    }
    FormulaIndex lefti = f->get_left_index();
    FormulaIndex righti = f->get_right_index();

    // check if k1 says that left is dead
    if ((kbentries[ki1]->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[ki1]->get_first() != lefti)) {
        return false;
    }

    // check if k2 says that right is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
        (kbentries[ki2]->get_first() != righti)) {
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that f=S is dead based on k1 (S \subseteq S') and k2 (S' is dead)
bool ProofChecker::check_rule_D3(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2) {

    // check if k1 says that f is a subset of "x" (x can be anything)
    if ((kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) ||
       (kbentries[ki1]->get_first() != fi)) {
        return false;
    }

    FormulaIndex xi = kbentries[ki1]->get_second();

    // check if k2 says that x is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != xi)) {
        return false;
    }
    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that the task is unsolvable based on k ({I} is dead)
bool ProofChecker::check_rule_D4(KnowledgeIndex newki, KnowledgeIndex ki) {

    // check that k says that {I} is dead
    if (kbentries[ki]->get_kbentry_type() != KBType::DEAD) {
        return false;
    }
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki]->get_first()]);
    if ((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        return false;
    }

    add_kbentry(new KBEntryUnsolvable(), newki);
    unsolvability_proven = true;
    return true;
}

// KBEntry newki says that the task is unsolvable based on k (S_G(\Pi) is dead)
bool ProofChecker::check_rule_D5(KnowledgeIndex newki, KnowledgeIndex ki) {

    // check that k says that S_G(\Pi) is dead
    if (kbentries[ki]->get_kbentry_type() != KBType::DEAD) {
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki]->get_first()]);
    if ( (!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
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

    // check if k1 says that S[A] \subseteq S \cup S'
    if (kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) {
        return false;
    }
    // check if the left side of k1 is S[A]
    SetFormulaProgression *s_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki1]->get_first()]);
    if ((!s_prog) || (s_prog->get_subformula_index() != fi)) {
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()]);
    if((!s_cup_sp) || (s_cup_sp->get_left_index() != fi)) {
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_index();

    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != spi)) {
        return false;
    }

    // check if k3 says that S \cap S_G(\Pi) is dead
    if (kbentries[ki3]->get_kbentry_type() != KBType::DEAD) {
        return false;
    }
    SetFormulaIntersection *s_and_goal =
            dynamic_cast<SetFormulaIntersection *>(formulas[kbentries[ki3]->get_first()]);
    // check if left side of s_and goal is S
    if ((!s_and_goal) || (s_and_goal->get_left_index() != fi)) {
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_and_goal->get_right_index()]);
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that S_not is dead based on k1 (S[A] \subseteq S \cup S'),
// k2 (S' is dead) and k3 ({I} \subseteq S_not)
bool ProofChecker::check_rule_D7(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {

    // check if fi corresponds to s_not
    SetFormulaNegation *s_not = dynamic_cast<SetFormulaNegation *>(formulas[fi]);
    if(!s_not) {
        return false;
    }
    FormulaIndex si = s_not->get_subformula_index();

    // check if k1 says that S[A] \subseteq S \cup S'
    if (kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) {
        return false;
    }
    // check if the left side of k1 is S[A]
    SetFormulaProgression *s_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki1]->get_first()]);
    if ((!s_prog) || (s_prog->get_subformula_index() != si)) {
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()]);
    if((!s_cup_sp) || (s_cup_sp->get_left_index() != si)) {
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_index();

    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != spi)) {
        return false;
    }

    // check if k3 says that {I} \subseteq S
    if (kbentries[ki3]->get_kbentry_type() != KBType::SUBSET) {
        return false;
    }
    // check that left side of k3 is {I}
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki3]->get_first()]);
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        return false;
    }
    // check that right side of k3 is S
    if(kbentries[ki3]->get_second() != si) {
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that S_not is dead based on k1 ([A]S \subseteq S \cup S'),
// k2 (S' is dead) and k3  (S_not \cap S_G(\Pi) is dead)
bool ProofChecker::check_rule_D8(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {

    // check if fi corresponds to s_not
    SetFormulaNegation *s_not = dynamic_cast<SetFormulaNegation *>(formulas[fi]);
    if(!s_not) {
        return false;
    }
    FormulaIndex si = s_not->get_subformula_index();

    // check if k1 says that [A]S \subseteq S \cup S'
    if(kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) {
        return false;
    }
    // check if the left side of k1 is [A]S
    SetFormulaRegression *s_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki1]->get_first()]);
    if ((!s_reg) || (s_reg->get_subformula_index() != si)) {
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()]);
    if((!s_cup_sp) || (s_cup_sp->get_left_index() != si)) {
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_index();

    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != spi)) {
        return false;
    }

    // check if k3 says that S_not \cap S_G(\Pi) is dead
    if (kbentries[ki3]->get_kbentry_type() != KBType::DEAD) {
        return false;
    }
    SetFormulaIntersection *s_not_and_goal =
            dynamic_cast<SetFormulaIntersection *>(formulas[kbentries[ki3]->get_first()]);
    // check if left side of s_not_and goal is S_not
    if ((!s_not_and_goal) || (s_not_and_goal->get_left_index() != fi)) {
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[s_not_and_goal->get_right_index()]);
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that S is dead based on k1 ([A]S \subseteq S \cup S'),
// k2 (S' is dead) and k3 ({I} \subseteq S_not)
bool ProofChecker::check_rule_D9(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {

    // check if k1 says that [A]S \subseteq S \cup S'
    if(kbentries[ki1]->get_kbentry_type() != KBType::SUBSET) {
        return false;
    }
    // check if the left side of k1 is [A]S
    SetFormulaRegression *s_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki1]->get_first()]);
    if ((!s_reg) || (s_reg->get_subformula_index() != fi)) {
        return false;
    }
    // check f the right side of k1 is S \cup S'
    SetFormulaUnion *s_cup_sp =
            dynamic_cast<SetFormulaUnion *>(formulas[kbentries[ki1]->get_second()]);
    if((!s_cup_sp) || (s_cup_sp->get_left_index() != fi)) {
        return false;
    }

    FormulaIndex spi = s_cup_sp->get_right_index();

    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != spi)) {
        return false;
    }

    // check if k3 says that {I} \subseteq S_not
    if (kbentries[ki3]->get_kbentry_type() != KBType::SUBSET) {
        return false;
    }
    // check that left side of k3 is {I}
    SetFormulaConstant *init =
            dynamic_cast<SetFormulaConstant *>(formulas[kbentries[ki3]->get_first()]);
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        return false;
    }
    // check that right side of k3 is S_not
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[kbentries[ki3]->get_second()]);
    if((!s_not) || s_not->get_subformula_index() != fi) {
        return false;
    }

    add_kbentry(new KBEntryDead(fi), newki);
    return true;
}


// KBEntry newki says that S'_not[A] \subseteq S_not based on k ([A]S \subseteq S')
bool ProofChecker::check_rule_D10(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                                KnowledgeIndex ki) {

    // check that f1 represents S'_not[A] and f2 S_not
    SetFormulaProgression *sp_not_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[fi1]);
    if(!sp_not_prog) {
        return false;
    }
    SetFormulaNegation *sp_not =
            dynamic_cast<SetFormulaNegation *>(formulas[sp_not_prog->get_subformula_index()]);
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[fi2]);
    if(!sp_not || !s_not) {
        return false;
    }

    FormulaIndex si = s_not->get_subformula_index();
    FormulaIndex spi = sp_not->get_subformula_index();

    // check if k says that [A]S \subseteq S'
    if(kbentries[ki]->get_kbentry_type() != KBType::SUBSET) {
        return false;
    }
    SetFormulaRegression *s_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[kbentries[ki]->get_first()]);
    if(!s_reg || s_reg->get_subformula_index() != si || kbentries[ki]->get_second() != spi) {
        return false;
    }
    add_kbentry(new KBEntrySubset(fi1, fi2), newki);
    return true;
}


// KBEntry newki says that [A]S'_not \subseteq S_not based on k (S[A] \subseteq S')
bool ProofChecker::check_rule_D11(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                                KnowledgeIndex ki) {

    //check that f1 represents [A]S'_not and f_2 S_not
    SetFormulaRegression *sp_not_reg =
            dynamic_cast<SetFormulaRegression *>(formulas[fi1]);
    if(!sp_not_reg) {
        return false;
    }
    SetFormulaNegation *sp_not =
            dynamic_cast<SetFormulaNegation *>(formulas[sp_not_reg->get_subformula_index()]);
    SetFormulaNegation *s_not =
            dynamic_cast<SetFormulaNegation *>(formulas[fi2]);
    if(!sp_not || !s_not) {
        return false;
    }

    FormulaIndex si = s_not->get_subformula_index();
    FormulaIndex spi = sp_not->get_subformula_index();

    // check if k says that S[A] \subseteq S'
    if(kbentries[ki]->get_kbentry_type() != KBType::SUBSET) {
        return false;
    }
    SetFormulaProgression *s_prog =
            dynamic_cast<SetFormulaProgression *>(formulas[kbentries[ki]->get_first()]);
    if(!s_prog || s_prog->get_subformula_index() != si || kbentries[ki]->get_second() != spi) {
        return false;
    }
    add_kbentry(new KBEntrySubset(fi1, fi2), newki);
    return true;
}


// check if L \subseteq L'
bool ProofChecker::check_statement_B1(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {

    FormulaIndex l_resolved = fi1;
    FormulaIndex lp_resolved = fi2;

    // resolve negated formulas and instead pass bools if the formulas are negated
    bool left_negated = false;
    bool right_negated = false;
    SetFormulaNegation *neg = dynamic_cast<SetFormulaNegation *>(formulas[fi1]);
    if(neg) {
        l_resolved = neg->get_subformula_index();
        left_negated = true;
    }
    neg = dynamic_cast<SetFormulaNegation *>(formulas[fi2]);
    if(neg) {
        lp_resolved = neg->get_subformula_index();
        right_negated = true;
    }

    if(formulas[l_resolved]->is_subset(formulas[lp_resolved], left_negated, right_negated)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        return true;
    }
    return false;
}

// check if X \subseteq X' \cup X''
bool ProofChecker::check_statement_B2(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {

    // check if fi2 represents X' \cup X''
    SetFormulaUnion *xp_cup_xpp = dynamic_cast<SetFormulaUnion *>(formulas[fi2]);
    if(!xp_cup_xpp) {
        std::cerr << "trying to apply B2 when right side is not a union" << std::endl;
        return false;
    }
    FormulaIndex xpi = xp_cup_xpp->get_left_index();
    FormulaIndex xppi = xp_cup_xpp->get_right_index();

    if(formulas[fi1]->is_subset(formulas[xpi], formulas[xppi])) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        return true;
    }
    return false;
}

// check if L \cap S_G(\Pi) \subseteq L'
bool ProofChecker::check_statement_B3(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {

    // check if fi1 represents L \cap S_G(\Pi)
    SetFormulaIntersection *l_cap_goal = dynamic_cast<SetFormulaIntersection *>(formulas[fi1]);
    if(!l_cap_goal) {
        return false;
    }
    SetFormulaConstant *goal =
            dynamic_cast<SetFormulaConstant *>(formulas[l_cap_goal->get_right_index()]);
    if(!goal) {
        return false;
    }

    FormulaIndex l_resolved = l_cap_goal->get_left_index();
    FormulaIndex lp_resolved = fi2;

    // resolve negated formulas and instead pass bools if the formulas are negated
    bool left_negated = false;
    bool right_negated = false;
    SetFormulaNegation *neg = dynamic_cast<SetFormulaNegation *>(formulas[l_resolved]);
    if(neg) {
        l_resolved = neg->get_subformula_index();
        left_negated = true;
    }
    neg = dynamic_cast<SetFormulaNegation *>(formulas[fi2]);
    if(neg) {
        lp_resolved = neg->get_subformula_index();
        right_negated = true;
    }

    if(formulas[l_resolved]->intersection_with_goal_is_subset(formulas[lp_resolved], left_negated, right_negated)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        return true;
    }
    return false;
}


// check if X[A] \subseteq X \cup L
bool ProofChecker::check_statement_B4(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {

    // check if fi1 represents X[A]
    SetFormulaProgression *x_prog = dynamic_cast<SetFormulaProgression *>(formulas[fi1]);
    if(!x_prog) {
        return false;
    }
    FormulaIndex xi = x_prog->get_subformula_index();

    //check if fi2 represents X \cup L
    SetFormulaUnion *x_cup_l = dynamic_cast<SetFormulaUnion *>(formulas[fi2]);
    if((!x_cup_l) || (x_cup_l->get_left_index() != xi)) {
        return false;
    }
    FormulaIndex l_resolved = x_cup_l->get_right_index();

    //resolve negated formula and instead pass bool if the formula is negated
    bool negated = false;
    SetFormulaNegation *neg = dynamic_cast<SetFormulaNegation *>(formulas[l_resolved]);
    if(neg) {
        l_resolved = neg->get_subformula_index();
        negated = true;
    }

    if(formulas[xi]->progression_is_union_subset(formulas[l_resolved], negated)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        return true;
    }
    return false;
}


// check if [A]X \subseteq X \cup L
bool ProofChecker::check_statement_B5(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2) {

    // check if fi1 represents [A]X
    SetFormulaRegression *x_reg = dynamic_cast<SetFormulaRegression *>(formulas[fi1]);
    if(!x_reg) {
        return false;
    }
    FormulaIndex xi = x_reg->get_subformula_index();

    //check if fi2 represents X \cup L
    SetFormulaUnion *x_cup_l = dynamic_cast<SetFormulaUnion *>(formulas[fi2]);
    if((!x_cup_l) || (x_cup_l->get_left_index() != xi)) {
        return false;
    }
    FormulaIndex l_resolved = x_cup_l->get_right_index();

    //resolve negated formula and instead pass bool if the formula is negated
    bool negated = false;
    SetFormulaNegation *neg = dynamic_cast<SetFormulaNegation *>(formulas[l_resolved]);
    if(neg) {
        l_resolved = neg->get_subformula_index();
        negated = true;
    }

    if(formulas[xi]->regression_is_union_subset(formulas[l_resolved], negated)) {
        add_kbentry(new KBEntrySubset(fi1, fi2), newki);
        return true;
    }
    return false;
}


bool ProofChecker::is_unsolvability_proven() {
    return unsolvability_proven;
}
