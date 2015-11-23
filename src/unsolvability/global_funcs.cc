#include <sstream>
#include <stdlib.h>

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
    std::cout << "INFO: " << info << " [" << *timer << "]" << std::endl;
}

void initialize_timer() {
    timer = new Timer();
}
