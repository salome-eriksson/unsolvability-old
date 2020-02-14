#ifndef KNOWLEDGE_H
#define KNOWLEDGE_H

enum class SetType {
    ACTION,
    STATE
};

class Knowledge
{
public:
    virtual ~Knowledge() = 0;
};

class SubsetKnowledge : public Knowledge
{
private:
    SetType type;
    int left_id;
    int right_id;
public:
    SubsetKnowledge(int left_id, int right_id);
    virtual ~SubsetKnowledge() {}

    int get_left_id();
    int get_right_id();
};

class DeadKnowledge : public Knowledge
{
private:
    int set_id;
public:
    DeadKnowledge(int set_id);
    virtual ~DeadKnowledge() {}

    int get_set_id();
};

// TODO: an empty class seems pretty pointless
class UnsolvableKnowledge : public Knowledge
{
public:
    virtual ~UnsolvableKnowledge() {}
};

#endif // KNOWLEDGE_H
