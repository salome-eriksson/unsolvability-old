#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <limits>

#include "global_funcs.h"

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
    exit(0);
}

void print_info(std::string info) {
    std::cout << "INFO: " << info << " [" << timer << "]" << std::endl;
}

void initialize_timer() {
    timer = Timer();
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
