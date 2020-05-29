#ifndef STATESETCOMPOSITIONS_H
#define STATESETCOMPOSITIONS_H

#include "setcompositions.h"
#include "stateset.h"


/*
 * COLLECTION OF ALL STATESET CLASSES THAT DERIVE FROM
 * SOME INTERFACE FROM SETCOMPOSITONS
 */


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
    virtual bool gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                        std::vector<StateSetVariable *> &positive,
                                        std::vector<StateSetVariable *> &negative,
                                        bool must_be_variable = false) override;
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
    virtual bool gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                               std::vector<StateSetVariable *> &positive,
                                               std::vector<StateSetVariable *> &negative,
                                               bool must_be_variable = false) override;
};

class StateSetNegation : public StateSet, public SetNegation
{
private:
    int id_child;
public:
    StateSetNegation(std::stringstream &input, Task &task);
    virtual ~StateSetNegation() {}

    virtual int get_child_id();
    virtual bool gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                        std::vector<StateSetVariable *> &positive,
                                        std::vector<StateSetVariable *> &negative,
                                        bool must_be_variable = false) override;
    virtual bool gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                               std::vector<StateSetVariable *> &positive,
                                               std::vector<StateSetVariable *> &negative,
                                               bool must_be_variable = false) override;
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
};


#endif // STATESETCOMPOSITIONS_H
