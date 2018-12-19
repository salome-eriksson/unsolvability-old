#ifndef ACTIONSET_H
#define ACTIONSET_H

#include "task.h"

#include <unordered_set>

class ActionSet
{
public:
    ActionSet();
    virtual ~ActionSet() {}
    virtual bool contains(int ai) = 0;
    virtual bool is_subset(ActionSet *other) = 0;
    virtual void get_actions(std::unordered_set<int> &set) = 0;
    virtual bool is_constantall() = 0;
};

class ActionSetBasic : public ActionSet
{
private:
    std::unordered_set<int> action_indices;
public:
    ActionSetBasic(std::unordered_set<int> &action_indices);
    virtual bool contains(int ai);
    virtual bool is_subset(ActionSet *other);
    virtual void get_actions(std::unordered_set<int> &set);
    virtual bool is_constantall();
};

class ActionSetUnion : public ActionSet
{
private:
    ActionSet *left;
    ActionSet *right;
public:
    ActionSetUnion(ActionSet *left, ActionSet *right);
    virtual bool contains(int ai);
    virtual bool is_subset(ActionSet *other);
    virtual void get_actions(std::unordered_set<int> &set);
    virtual bool is_constantall();
};

class ActionSetConstantAll : public ActionSet
{
    int action_amount;
public:
    ActionSetConstantAll(Task *task);
    virtual bool contains(int ai);
    virtual bool is_subset(ActionSet *other);
    virtual void get_actions(std::unordered_set<int> &set);
    virtual bool is_constantall();
};

#endif // ACTIONSET_H
