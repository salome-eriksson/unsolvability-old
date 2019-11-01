#ifndef GLOBAL_FUNCS_H
#define GLOBAL_FUNCS_H

#include "cuddObj.hh"

#include "timer.h"
#include "task.h"

extern Timer timer;
extern int g_timeout;
extern bool g_discard_formulas;
extern Cudd manager;

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

void initialize_timer();
void set_timeout(int x);
void set_discard_formulas(bool b);
int get_peak_memory_in_kb(bool use_buffered_input = true);
void exit_with(ExitCode code);
void exit_oom(size_t size);
void exit_timeout(std::string);
void register_event_handlers();


#endif /* GLOBAL_FUNCS_H */
