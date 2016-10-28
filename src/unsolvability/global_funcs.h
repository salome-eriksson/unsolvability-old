#ifndef GLOBAL_FUNCS_H
#define GLOBAL_FUNCS_H

#include <iostream>
#include <vector>

#include "timer.h"

extern Timer timer;
extern int g_timeout;

enum class ExitCode {
    CERTIFICATE_VALID = 0,
    CRITICAL_ERROR = 1,
    CERTIFICATE_NOT_VALID = 2,
    NO_TASK_FILE= 3,
    NO_CERTIFICATE_FILE = 4,
    PARSING_ERROR = 5,
    OUT_OF_MEMORY = 6,
    TIMEOUT = 7
};

//TODO: move this to an appropriate place
void split(const std::string &s, std::vector<std::string> &vec, char delim);
void print_parsing_error_and_exit(std::string &line, std::string expected);
void print_info(std::string info);
void initialize_timer();
void set_timeout(int x);
int get_peak_memory_in_kb(bool use_buffered_input = true);
void exit_with(ExitCode code);
void exit_oom(long size);
void exit_timeout(std::string);



#endif /* GLOBAL_FUNCS_H */
