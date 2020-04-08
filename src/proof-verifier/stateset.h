#ifndef STATESET_H
#define STATESET_H

#include "setcompositions.h"
#include "actionset.h"

#include <functional>
#include <map>
#include <memory>
#include <sstream>

// TODO: remove eventually?
enum class SetFormulaType {
    CONSTANT,
    BDD,
    HORN,
    TWOCNF,
    EXPLICIT,
    NEGATION,
    INTERSECTION,
    UNION,
    PROGRESSION,
    REGRESSION
};
enum class ConstantType {
    EMPTY,
    GOAL,
    INIT
};

class StateSet;
typedef std::function<std::unique_ptr<StateSet>(std::stringstream &input, Task &task)> StateSetConstructor;


class StateSet
{
public:
    virtual ~StateSet() {}

    static std::map<std::string, StateSetConstructor> *get_stateset_constructors();

   // TODO: can we get rid of this method?
   // (then we need to make the destructor pure virtual)
    virtual SetFormulaType get_formula_type() = 0;
};


class StateSetVariable : public StateSet
{
public:
    /*
     * We expect the calling method to eliminate negations:
     * For example if the calling method tries to verify
     * ((not S) \cap S') \subseteq (S'' \cup (not S'''))
     * it should call with parameters left = <S',S'''> and
     * right = <S,S''>
     */
    virtual bool check_statement_b1(std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right) = 0;
    virtual bool check_statement_b2(std::vector<StateSetVariable *> &progress,
                                    std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices) = 0;
    virtual bool check_statement_b3(std::vector<StateSetVariable *> &regress,
                                    std::vector<StateSetVariable *> &left,
                                    std::vector<StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices) = 0;
    virtual bool check_statement_b4(StateSetVariable *right, bool left_positive,
                                    bool right_positive) = 0;

    // TODO: can we remove this method?
    virtual const std::vector<int> &get_varorder() = 0;

    virtual bool supports_mo() = 0;
    virtual bool supports_ce() = 0;
    virtual bool supports_im() = 0;
    virtual bool supports_me() = 0;
    virtual bool supports_todnf() = 0;
    virtual bool supports_tocnf() = 0;
    virtual bool supports_ct() = 0;
    virtual bool is_nonsuccint() = 0;

    // TODO: remodel this
    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const = 0;
    virtual bool is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) = 0;
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) = 0;
    // returns false if no clause with index i exists
    virtual bool get_clause(int i, std::vector<int> &varorder, std::vector<bool> &clause) = 0;
    virtual int get_model_count() = 0;
};


class StateSetUnion : public StateSet, public SetUnion
{
private:
    int id_left;
    int id_right;
public:
    StateSetUnion(std::stringstream &input, Task &task);
    virtual ~StateSetUnion() {}

    virtual int get_left_id();
    virtual int get_right_id();
    virtual SetFormulaType get_formula_type();
};

class StateSetIntersection : public StateSet, public SetIntersection
{
private:
    int id_left;
    int id_right;
public:
    StateSetIntersection(std::stringstream &input, Task &task);
    virtual ~StateSetIntersection() {}

    virtual int get_left_id();
    virtual int get_right_id();
    virtual SetFormulaType get_formula_type();
};

class StateSetNegation : public StateSet, public SetNegation
{
private:
    int id_child;
public:
    StateSetNegation(std::stringstream &input, Task &task);
    virtual ~StateSetNegation() {}

    virtual int get_child_id();
    virtual SetFormulaType get_formula_type();
};

class StateSetProgression : public StateSet
{
private:
    int id_stateset;
    int id_actionset;
public:
    StateSetProgression(std::stringstream &input, Task &task);
    virtual ~StateSetProgression() {}

    virtual int get_stateset_id();
    virtual int get_actionset_id();
    virtual SetFormulaType get_formula_type();
};

class StateSetRegression : public StateSet
{
private:
    int id_stateset;
    int id_actionset;
public:
    StateSetRegression(std::stringstream &input, Task &task);
    virtual ~StateSetRegression() {}

    virtual int get_stateset_id();
    virtual int get_actionset_id();
    virtual SetFormulaType get_formula_type();
};


template<class T>
class StateSetBuilder {
public:
    StateSetBuilder(std::string key) {
        StateSetConstructor constructor = [](std::stringstream &input, Task &task) -> std::unique_ptr<StateSet> {
            return std::unique_ptr<T>(new T(input, task));
        };
        StateSet::get_stateset_constructors()->insert(std::make_pair(key, constructor));
    }

    ~StateSetBuilder() = default;
    StateSetBuilder(const StateSetBuilder<T> &other) = delete;
};
#endif // STATESET_H
