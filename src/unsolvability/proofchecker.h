#ifndef PROOFCHECKER_H
#define PROOFCHECKER_H

#include "setformula.h"

#include <iostream>
#include <deque>

typedef int KnowledgeIndex;

enum KBType {
    SUBSET,
    DEAD,
    UNSOLVABLE
};

class KBEntry {
public:
    static const int INDEXNONE = -1;
    KBEntry();
    virtual FormulaIndex get_first() = 0;
    virtual FormulaIndex get_second() = 0;

    virtual KBType get_kbentry_type() = 0;
};

class KBEntrySubset : public KBEntry {
private:
    FormulaIndex first;
    FormulaIndex second;
public:
    KBEntrySubset(FormulaIndex first, FormulaIndex second)
        : first(first), second(second) {}
    virtual FormulaIndex get_first() { return first; }
    virtual FormulaIndex get_second() { return second; }
    virtual KBType get_kbentry_type() { return KBType::SUBSET; }
};

class KBEntryDead : public KBEntry {
private:
    FormulaIndex first;
public:
    KBEntryDead(FormulaIndex first) : first(first) {}
    virtual FormulaIndex get_first() { return first; }
    virtual FormulaIndex get_second() { return INDEXNONE; }
    virtual KBType get_kbentry_type() { return KBType::DEAD; }
};

class KBEntryUnsolvable : public KBEntry {
public:
    KBEntryUnsolvable() {}
    virtual FormulaIndex get_first() { return INDEXNONE; }
    virtual FormulaIndex get_second() { return INDEXNONE; }
    virtual KBType get_kbentry_type() { return KBType::UNSOLVABLE; }
};

class ProofChecker
{
private:
    std::deque<SetFormula *> formulas;
    std::deque<KBEntry *> kbentries;
    bool unsolvability_proven;
public:
    ProofChecker();
    // TODO: rather ints? or vectors? or the entire line and add parsing?
    bool checkRuleD1(KnowledgeIndex newki, FormulaIndex emptyi);
    bool checkRuleD2(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2);
    bool checkRuleD3(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2);
    bool checkRuleD4(KnowledgeIndex newki, KnowledgeIndex ki);
    bool checkRuleD5(KnowledgeIndex newki, KnowledgeIndex ki);
    bool checkRuleD6(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3);
    bool checkRuleD7(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3);
    bool checkRuleD8(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3);
    bool checkRuleD9(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3);
    bool checkRuleD10(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                      KnowledgeIndex ki1, KnowledgeIndex ki2);
    bool checkRuleD11(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                      KnowledgeIndex ki1, KnowledgeIndex ki2);

    bool checkStatementB1(KnowledgeIndex newki, FormulaIndex f1, FormulaIndex f2);
    bool checkStatementB2(KnowledgeIndex newki, FormulaIndex f1, FormulaIndex f2);
    bool checkStatementB3(KnowledgeIndex newki, FormulaIndex f1, FormulaIndex f2);
    bool checkStatementB4(KnowledgeIndex newki, FormulaIndex f1, FormulaIndex f2);
    bool checkStatementB5(KnowledgeIndex newki, FormulaIndex f1, FormulaIndex f2);
};

#endif // PROOFCHECKER_H
