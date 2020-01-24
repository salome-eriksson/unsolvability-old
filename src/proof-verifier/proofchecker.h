#ifndef PROOFCHECKER_H
#define PROOFCHECKER_H

#include "actionset.h"
#include "stateset.h"
#include "task.h"

#include <deque>
#include <iostream>
#include <map>
#include <memory>

typedef int KnowledgeIndex;

enum class KBType {
    SUBSET,
    DEAD,
    ACTION,
    UNSOLVABLE
};

enum class ExpressionType {
    BDD,
    HORN,
    DUALHORN,
    EXPLICIT,
    TWOCNF,
    CONSTANT,
    NEGATION,
    INTERSECTION,
    UNION,
    PROGRESSION,
    REGRESSION,
    NONE
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

class KBEntryAction : public KBEntry {
private:
    ActionSetIndex first;
    ActionSetIndex second;
public:
    KBEntryAction(FormulaIndex first, FormulaIndex second)
        : first(first), second(second) {}
    virtual FormulaIndex get_first() { return first; }
    virtual FormulaIndex get_second() { return second; }
    virtual KBType get_kbentry_type() { return KBType::ACTION; }
};

class KBEntryUnsolvable : public KBEntry {
public:
    KBEntryUnsolvable() {}
    virtual FormulaIndex get_first() { return INDEXNONE; }
    virtual FormulaIndex get_second() { return INDEXNONE; }
    virtual KBType get_kbentry_type() { return KBType::UNSOLVABLE; }
};

struct FormulaEntry {
    std::unique_ptr<StateSet> fpointer;
    // the last KnowledgeIndex where we need the actual set representation
    KnowledgeIndex last_occ;
    FormulaEntry(std::unique_ptr<StateSetVariable> fpointer, int last_occ)
        : fpointer(std::move(fpointer)), last_occ(last_occ) {}
    FormulaEntry()
        : last_occ(-1) {}
};

class ProofChecker
{
private:
    std::deque<FormulaEntry> formulas;
    std::deque<std::unique_ptr<KBEntry>> kbentries;
    std::deque<std::unique_ptr<ActionSet>> actionsets;
    bool unsolvability_proven;

    std::map<std::string, ExpressionType> expression_types;

    void add_kbentry(std::unique_ptr<KBEntry> entry, KnowledgeIndex index);
    void remove_formulas_if_obsolete(std::vector<int> indices, int current_ki);

    /*
     * The return formula serves as a reference which basic formula type is involved.
     * If it is null, then the set is not an intersection of set literals of the same type.
     * If it is a constant formula, then all set variables involved are constant.
     * If it is a concrete type, then all set variables invovled are of this type or constant.
     */
    StateSetVariable *gather_sets_intersection(StateSet *f,
                                 std::vector<StateSetVariable *> &positive,
                                 std::vector<StateSetVariable *> &negative);
    StateSetVariable *gather_sets_union(StateSet *f,
                           std::vector<StateSetVariable *>&positive,
                           std::vector<StateSetVariable *>&negative);
    StateSetVariable *update_reference_and_check_consistency(StateSetVariable *reference_formula,
                                                       StateSetVariable *tmp, std::string stmt);
public:
    ProofChecker();

    void add_expression(std::ifstream &in, Task *task);
    // TODO one function for both types of actionsets would be nicer...
    void add_actionset(std::unique_ptr<ActionSet> actionset, ActionSetIndex index);
    void add_actionset_union(ActionSetIndex left, ActionSetIndex right, ActionSetIndex index);
    void first_pass(std::string certfile);

    // rules for checking if state sets are dead
    bool check_rule_ed(KnowledgeIndex conclusion, FormulaIndex set);
    bool check_rule_ud(KnowledgeIndex conclusion, FormulaIndex set,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);
    bool check_rule_sd(KnowledgeIndex conclusion, FormulaIndex set,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);
    bool check_rule_pg(KnowledgeIndex conclusion, FormulaIndex set,
                       KnowledgeIndex premise1, KnowledgeIndex premise2,
                       KnowledgeIndex premise3);
    bool check_rule_pi(KnowledgeIndex conclusion, FormulaIndex set,
                       KnowledgeIndex premise1, KnowledgeIndex premise2,
                       KnowledgeIndex premise3);
    bool check_rule_rg(KnowledgeIndex conclusion, FormulaIndex set,
                       KnowledgeIndex premise1, KnowledgeIndex premise2,
                       KnowledgeIndex premise3);
    bool check_rule_ri(KnowledgeIndex conclusion, FormulaIndex set,
                       KnowledgeIndex premise1, KnowledgeIndex premise2,
                       KnowledgeIndex premise3);

    // rules for ending the proof
    bool check_rule_ci(KnowledgeIndex conclusion, KnowledgeIndex premise);
    bool check_rule_cg(KnowledgeIndex conclusion, KnowledgeIndex premise);


    // rules from basic set theory
    bool check_rule_ur(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_rule_ul(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_rule_ir(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_rule_il(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_rule_di(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_rule_su(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);
    bool check_rule_si(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);
    bool check_rule_st(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);

    // rules for progression and its relation to regression


    bool check_rule_at(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);
    bool check_rule_au(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);
    bool check_rule_pt(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);
    bool check_rule_pu(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                       KnowledgeIndex premise1, KnowledgeIndex premise2);
    bool check_rule_rp(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                      KnowledgeIndex premise);
    bool check_rule_pr(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right,
                      KnowledgeIndex premise);

    bool check_statement_B1(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_statement_B2(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_statement_B3(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_statement_B4(KnowledgeIndex conclusion, FormulaIndex left, FormulaIndex right);
    bool check_statement_B5(KnowledgeIndex conclusion, ActionSetIndex left, ActionSetIndex right);

    bool is_unsolvability_proven();
};

#endif // PROOFCHECKER_H
