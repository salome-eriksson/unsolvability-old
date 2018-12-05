#ifndef PROOFCHECKER_H
#define PROOFCHECKER_H

#include "setformula.h"

#include <iostream>
#include <deque>
#include <memory>

typedef int KnowledgeIndex;

enum class KBType {
    SUBSET,
    DEAD,
    UNSOLVABLE
};

// TODO: virtual get_first() and get_second() methods are somewhat hacky
// but it saves us dynamic casts
class KBEntry {
public:
    static const FormulaIndex INDEXNONE = -1;
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

struct FormulaEntry {
    std::unique_ptr<SetFormula> fpointer;
    // the last KnowledgeIndex where we need the actual set representation
    KnowledgeIndex last_occ;
    FormulaEntry(std::unique_ptr<SetFormula> fpointer, int last_occ)
        : fpointer(std::move(fpointer)), last_occ(last_occ) {}
    FormulaEntry()
        : last_occ(-1) {}
};

class ProofChecker
{
private:
    std::deque<FormulaEntry> formulas;
    std::deque<std::unique_ptr<KBEntry>> kbentries;
    bool unsolvability_proven;

    void add_kbentry(std::unique_ptr<KBEntry> entry, KnowledgeIndex index);
    void remove_formulas_if_obsolete(std::vector<int> indices, int current_ki);
public:
    ProofChecker();

    void add_formula(std::unique_ptr<SetFormula> formula, FormulaIndex index);
    void first_pass(std::string certfile);

    bool check_rule_D1(KnowledgeIndex newki, FormulaIndex emptyi);
    bool check_rule_D2(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2);
    bool check_rule_D3(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2);
    bool check_rule_D4(KnowledgeIndex newki, KnowledgeIndex ki);
    bool check_rule_D5(KnowledgeIndex newki, KnowledgeIndex ki);
    bool check_rule_D6(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3);
    bool check_rule_D7(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3);
    bool check_rule_D8(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3);
    bool check_rule_D9(KnowledgeIndex newki, FormulaIndex fi,
                     KnowledgeIndex ki1, KnowledgeIndex ki2, KnowledgeIndex ki3);
    bool check_rule_D10(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                      KnowledgeIndex ki);
    bool check_rule_D11(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2,
                      KnowledgeIndex ki);

    bool check_statement_B1(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2);
    bool check_statement_B2(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2);
    bool check_statement_B3(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2);
    bool check_statement_B4(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2);
    bool check_statement_B5(KnowledgeIndex newki, FormulaIndex fi1, FormulaIndex fi2);

    bool is_unsolvability_proven();
};

#endif // PROOFCHECKER_H