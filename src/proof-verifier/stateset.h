#ifndef STATESET_H
#define STATESET_H

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_set>

#include "task.h"


enum class ConstantType {
    EMPTY,
    GOAL,
    INIT
};

// TODO: can we avoid these forward declarations? I don't think so
class StateSet;
typedef std::function<std::unique_ptr<StateSet>(std::stringstream &input, Task &task)> StateSetConstructor;


class StateSetVariable;
class StateSet
{
public:
    virtual ~StateSet() = 0;

    static std::map<std::string, StateSetConstructor> *get_stateset_constructors();
    virtual bool gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                std::vector<StateSetVariable *> &positive,
                                std::vector<StateSetVariable *> &negative,
                                bool must_be_variable = false);
    virtual bool gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                std::vector<StateSetVariable *> &positive,
                                std::vector<StateSetVariable *> &negative,
                                bool must_be_variable = false);
};


class StateSetVariable : public StateSet
{
public:
    virtual ~StateSetVariable () = 0;
    //static std::map<std::string, StateSetConstructor> *get_stateset_constructors();
    virtual bool gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                        std::vector<StateSetVariable *> &positive,
                                        std::vector<StateSetVariable *> &negative,
                                        bool must_be_variable = false) override;
    virtual bool gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                               std::vector<StateSetVariable *> &positive,
                                               std::vector<StateSetVariable *> &negative,
                                               bool must_be_variable = false) override;
};


class StateSetFormalism : public StateSetVariable
{
public:
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
    virtual bool check_statement_b4(StateSetFormalism *right, bool left_positive,
                                    bool right_positive) = 0;

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

    // TODO: can we remove this method?
    virtual const std::vector<int> &get_varorder() = 0;


    /*
     * checks whether stateset is compatible with this (using covariance)
     *  - if yes, return a casted pointer to the subclass of this
     *  - if no, return nullpointer
     */
    virtual StateSetFormalism *get_compatible(StateSetVariable *stateset) = 0;
    /*
     * return a constant formula in the formalism of this (using covariance)
     */
    virtual StateSetFormalism *get_constant(ConstantType ctype) = 0;

    // TODO: think about error handling!
    // TOOD: do we need reference? after all we are calling it from a T * formula
    template <class T>
    std::vector<T *> convert_to_formalism(std::vector<StateSetVariable *> &vector, T *reference) {
        std::vector<T *>ret;
        ret.reserve(vector.size());
        for (StateSetVariable *formula : vector) {
            T *element = reference->get_compatible(formula);
            if (!element) {
                std::string msg = "could not convert vector to specific formalism, incompatible formulas!";
                throw std::runtime_error(msg);
            }
            ret.push_back(element);
        }
        return std::move(ret);
    }
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
