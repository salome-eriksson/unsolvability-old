#ifndef GLOBAL_FUNCS_H
#define GLOBAL_FUNCS_H

#include <iostream>
#include <vector>

#include "timer.h"

static Timer* timer=NULL;

//TODO: move this to an appropriate place
void split(const std::string &s, std::vector<std::string> &vec, char delim);
void print_parsing_error_and_exit(std::string &line, std::string expected);
void print_info(std::string info);
void initialize_timer();

#endif /* GLOBAL_FUNCS_H */
