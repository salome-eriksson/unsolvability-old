#ifndef GLOBAL_FUNCS_H
#define GLOBAL_FUNCS_H

#include <iostream>
#include <vector>

#include "timer.h"
#include "task.h"

extern Timer timer;
extern int g_timeout;
extern std::vector<std::vector<int>> hex;

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

enum class SpecialSet {
    EMPTYSET = 0,
    TRUESET = 1,
    GOALSET = 2,
    INITSET = 3
};

//TODO: move this to an appropriate place
void split(const std::string &s, std::vector<std::string> &vec, char delim);
void print_parsing_error_and_exit(std::string &line, std::string expected);
void print_info(std::string info);
void initialize_timer();
void set_timeout(int x);
int get_peak_memory_in_kb(bool use_buffered_input = true);
void exit_with(ExitCode code);
void exit_oom(size_t size);
void exit_timeout(std::string);
void register_event_handlers();
std::vector<std::string> determine_parameters(const std::string &parameter_line, char delim);
Cube parseCube(const std::string &param, int size);
std::string special_set_string(SpecialSet set);
void setup_hex();


#endif /* GLOBAL_FUNCS_H */
