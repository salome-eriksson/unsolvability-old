#ifndef ACTIONSET_H
#define ACTIONSET_H

#include "setcompositions.h"
#include "task.h"

#include <deque>
#include <memory>
#include <unordered_set>

class ActionSet
{
public:
    ActionSet();
    virtual ~ActionSet() {}
    virtual void get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices) = 0;
    virtual bool is_constantall() = 0;
};

class ActionSetBasic : public ActionSet
{
private:
    std::unordered_set<int> action_indices;
public:
    ActionSetBasic(std::unordered_set<int> &action_indices);
    virtual void get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices);
    virtual bool is_constantall();
};

class ActionSetUnion : public ActionSet, public SetUnion
{
private:
    int id_left;
    int id_right;
public:
    ActionSetUnion(int id_left, int id_right);
    virtual void get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices);
    virtual bool is_constantall();
    virtual int get_left_id();
    virtual int get_right_id();
};

class ActionSetConstantAll : public ActionSet
{
    std::unordered_set<int> action_indices;
public:
    ActionSetConstantAll(Task &task);
    virtual void get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices);
    virtual bool is_constantall();
};

#endif // ACTIONSET_H
