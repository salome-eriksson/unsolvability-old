#ifndef SETCOMPOSITIONS_H
#define SETCOMPOSITIONS_H

class SetUnion
{
public:
    virtual ~SetUnion() {}
    virtual int get_left_id() = 0;
    virtual int get_right_id() = 0;
};

class SetIntersection
{
public:
    virtual ~SetIntersection() {}
    virtual int get_left_id() = 0;
    virtual int get_right_id() = 0;
};

class SetNegation
{
public:
    virtual ~SetNegation() {}
    virtual int get_child_id() = 0;
};

#endif // SETCOMPOSITIONS_H
