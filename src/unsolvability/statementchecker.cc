#include "statementchecker.h"

// TODO: this is a duplicate from RuleChecker::determine_parameters
std::vector<std::string> StatementChecker::determine_parameters(const std::string &parameter_line) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = parameter_line.find(";", prev);
        if (pos == std::string::npos) {
            pos = parameter_line.length();
        }
        std::string token = parameter_line.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < parameter_line.length() && prev < parameter_line.length());
    return tokens;
}

// TOOD: this is a duplicate from RuleChecker::parseCube)
Cube StatementChecker::parseCube(const std::string &param) {
    Cube cube;
    cube.reserve(task->get_number_of_facts());
    std::istringstream iss(param);
    int n;
    while (iss >> n){
        cube.push_back(n);
    }
    assert(cube.size() == task->get_number_of_facts());
    return cube;
}
