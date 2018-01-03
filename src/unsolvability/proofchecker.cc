#include "proofchecker.h"

#include "setformulaconstant.h"
#include "setformulacompound.h"

ProofChecker::ProofChecker()
    : unsolvability_proven(false) {

}

// TODO: Code is VERY MESSY!!! (especially D6)

// KBEntry newki says that f=emptyset is dead
bool ProofChecker::checkRuleD1(KnowledgeIndex newki, FormulaIndex fi) {
    // TODO: is it a good idea to demand this? (concerns all methods)
    assert(kbentries.size() == newki);
    SetFormula *f = formulas[fi];
    if ((f->get_formula_type() != SetFormulaType::CONSTANT) ||
       (static_cast<SetFormulaConstant *>(f)->getConstantType() != ConstantType::EMPTY)) {
        return false;
    }
    kbentries.push_back(new KBEntryDead(fi));
    return true;
}


// KBEntry newki says that f=S \union S' is dead based on k1 (S is dead) and k2 (S' is dead)
bool ProofChecker::checkRuleD2(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2) {
    assert(kbentries.size() == newki);

    SetFormula *f = formulas[fi];
    if (f->get_formula_type() != SetFormulaType::UNION) {
        return false;
    }
    // f represents left \cup right
    FormulaIndex lefti = static_cast<SetFormulaUnion *>(f)->get_left_index();
    FormulaIndex righti = static_cast<SetFormulaUnion *>(f)->get_right_index();
    // check if k1 says that left is dead
    if ((kbentries[ki1]->get_kbentry_type != KBType::DEAD) ||
        (kbentries[ki1]->get_first() != lefti)) {
        return false;
    }
    // check if k2 says that right is dead
    if ((kbentries[ki2]->get_kbentry_type != KBType::DEAD) ||
        (kbentries[ki2]->get_first() != righti)) {
        return false;
    }
    kbentries.push_back(new KBEntryDead(fi));
    return true;
}


// KBEntry newki says that f=S is dead based on k1 (S \subseteq S') and k2 (S' is dead)
bool ProofChecker::checkRuleD3(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2) {
    assert(kbentries.size() == newki);

    // check if k1 says that f is a subset of "x" (x can be anything)
    if ((kbentries[ki1]->get_kbentry_type != KBType::SUBSET) ||
       (kbentries[ki1]->get_first() != fi)) {
        return false;
    }
    FormulaIndex xi = kbentries[ki1]->get_second();
    // check if k2 says that x is dead
    if ((kbentries[ki2]->get_kbentry_type != KBType::DEAD) ||
       (kbentries[ki2]->get_first() != xi)) {
        return false;
    }
    kbentries.push_back(new KBEntryDead(fi));
}


// KBEntry newki says that the task is unsolvable based on k ({I} is dead)
bool ProofChecker::checkRuleD4(KnowledgeIndex newki, KnowledgeIndex ki) {
    assert(kbentries.size() == newki);
    // check that k says that {I} is dead
    if (kbentries[ki]->get_kbentry_type != KBType::DEAD) {
        return false;
    }
    SetFormula *init = formulas[kbentries[ki]->get_first()];
    if ((init->get_formula_type() != SetFormulaType::CONSTANT) ||
       (static_cast<SetFormulaConstant *>(init)->getConstantType() != ConstantType::INIT)) {
        return false;
    }
    kbentries.push_back(new KBEntryUnsolvable());
    unsolvability_proven = true;
    return true;
}

// KBEntry newki says that the task is unsolvable based on k (S_G(\Pi) is dead)
bool ProofChecker::checkRuleD5(KnowledgeIndex newki, KnowledgeIndex ki) {
    assert(kbentries.size() == newki);
    // check that k says that S_G(\Pi) is dead
    if (kbentries[ki]->get_kbentry_type != KBType::DEAD) {
        return false;
    }
    SetFormula *goal = formulas[kbentries[ki]->get_first()];
    if ((goal->get_formula_type() != SetFormulaType::CONSTANT) ||
       (static_cast<SetFormulaConstant *>(goal)->getConstantType() != ConstantType::GOAL)) {
        return false;
    }
    kbentries.push_back(new KBEntryUnsolvable());
    unsolvability_proven = true;
    return true;
}


// KBEntry newki says that S is dead based on k1 (S[A] \subseteq S \cup S'),
// k2 (S' is dead) and k3 (S \cap S_G(\Pi) is dead)
bool ProofChecker::checkRuleD6(KnowledgeIndex newki, FormulaIndex fi,
                               KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3) {
    assert(kbentries.size() == newki);
    // check if k1 says that S[A] \subseteq S \cup S'
    if (kbentries[ki1]->get_kbentry_type != KBType::SUBSET) {
        return false;
    }
    // check if the left side of k1 is S[A]
    SetFormula* s_prog = formulas[kbentries[ki1]->get_first()];
    if ((s_prog->get_formula_type() != SetFormulaType::PROGRESSION) ||
       (static_cast<SetFormulaProgression *>(s_prog)->get_subformula_index() != fi)) {
        return false;
    }
    // spi is the index of S'
    FormulaIndex spi = kbentries[ki1]->get_second();
    // check if k2 says that S' is dead
    if ((kbentries[ki2]->get_kbentry_type() != KBType::DEAD) ||
       (kbentries[ki2]->get_first != spi)) {
        return false;
    }
    // check if k3 says that S \cap S_G(\Pi) is dead
    if (kbentries[ki3]->get_kbentry_type() != KBType::DEAD) {
        return false;
    }
    if (formulas[kbentries[ki3]->get_first()]->get_formula_type() != SetFormulaType::INTERSECTION) {
        return false;
    }
    SetFormulaIntersection *s_and_goal = static_cast<SetFormulaIntersection *>(formulas[kbentries[ki3]->get_first()]);
    if ((s_and_goal->get_left_index() != fi) ||
       (formulas[s_and_goal->get_right_index()]->get_formula_type() != SetFormulaType::CONSTANT)) {
        return false;
    }
    SetFormulaConstant *goal = static_cast<SetFormulaConstant>(formulas[s_and_goal->get_right_index()]);
    if(goal->getConstantType() != ConstantType::GOAL) {
        return false;
    }
    kbentries.push_back(new KBEntryDead(fi));
    return true;
}
