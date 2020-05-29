#ifndef SSVCONSTANT_H
#define SSVCONSTANT_H

#include "stateset.h"

#include <sstream>

class SSVConstant : public StateSetVariable
{
private:
    ConstantType constanttype;
    Task &task;

    StateSetVariable *find_first_non_constant_variable(std::vector<StateSetVariable *> &vec);
public:
    SSVConstant(std::stringstream &input, Task &task);
    ConstantType get_constant_type();
};

#endif // SSVCONSTANT_H
