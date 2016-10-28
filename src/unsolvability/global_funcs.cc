#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <limits>

#include "global_funcs.h"

Timer timer;
int g_timeout;

//TODO: move this to an appropriate place
void split(const std::string &s, std::vector<std::string> &vec, char delim) {
    vec.clear();
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        vec.push_back(item);
    }
}

void print_parsing_error_and_exit(std::string &line, std::string expected) {
    std::cout << "unexpected line in certificate: " << line
              << " (expected \"" << expected << "\")" << std::endl;
    exit_with(ExitCode::PARSING_ERROR);
}

void print_info(std::string info) {
    std::cout << "INFO: " << info << " [" << timer << "]" << std::endl;
}

void initialize_timer() {
    timer = Timer();
}

void set_timeout(int x) {
    g_timeout = x;
}

int get_peak_memory_in_kb(bool use_buffered_input) {
    // On error, produces a warning on cerr and returns -1.
    int memory_in_kb = -1;

    std::ifstream procfile;
    if (!use_buffered_input) {
        procfile.rdbuf()->pubsetbuf(0, 0);
    }
    procfile.open("/proc/self/status");
    std::string word;
    while (procfile.good()) {
        procfile >> word;
        if (word == "VmPeak:") {
            procfile >> memory_in_kb;
            break;
        }
        // Skip to end of line.
        procfile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    if (procfile.fail())
        memory_in_kb = -1;
    if (memory_in_kb == -1)
        std::cerr << "warning: could not determine peak memory" << std::endl;
    return memory_in_kb;
}

void exit_with(ExitCode code) {
    std::string msg;
    switch(code) {
    case ExitCode::CERTIFICATE_VALID:
        msg = "Exiting: certificate is valid";
        break;
    case ExitCode::CERTIFICATE_NOT_VALID:
        msg = "Exiting: certificate is not valid";
        break;
    case ExitCode::CRITICAL_ERROR:
        msg = "Exiting: unexplained critical error";
        break;
    case ExitCode::PARSING_ERROR:
        msg = "Exiting: parsing error";
        break;
    case ExitCode::NO_CERTIFICATE_FILE:
        msg = "Exiting: no certificate file found";
        break;
    case ExitCode::NO_TASK_FILE:
        msg = "Exiting: no task file found";
        break;
    case ExitCode::OUT_OF_MEMORY:
        msg = "Exiting: memory limit reached";
        break;
    case ExitCode::TIMEOUT:
        msg = "Exiting: timeout reached";
        break;
    default:
        msg = "Exiting: default exitcode";
        break;
    }
    std::cout << msg << std::endl;
    exit(static_cast<int>(code));
}

void exit_oom(long size) {
    std::cout << "abort memory " << get_peak_memory_in_kb() << "KB" << std::endl;
    std::cout << "abort time " << timer << std::endl;
    exit_with(ExitCode::OUT_OF_MEMORY);
}

void exit_timeout(std::string) {
    std::cout << "abort memory: " << get_peak_memory_in_kb() << "KB" << std::endl;
    std::cout << "abort time: " << timer << std::endl;
    exit_with(ExitCode::TIMEOUT);
}
