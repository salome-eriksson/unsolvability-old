#ifndef GLOBAL_FUNCS_H
#define GLOBAL_FUNCS_H

#include <iostream>
#include <vector>


//TODO: move this to an appropriate place
void split(const std::string &s, std::vector<std::string> &vec, char delim);
void print_parsing_error_and_exit(std::string &line, std::string expected);


#endif /* GLOBAL_FUNCS_H */
