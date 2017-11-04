#include "unsolvabilitymanager.h"
#include "../globals.h"

UnsolvabilityManager::UnsolvabilityManager()
    : setcount(0) {
    // for the maia grid, use a directory of the local file system
    char *sge_env = std::getenv("SLURM_JOBID");
    if(sge_env != NULL) {
        directory = std::string(std::getenv("TMPDIR")) + "/";
    }
    emptysetid = setcount++;
    truesetid = setcount++;
    goalsetid = setcount++;
    initsetid = setcount++;
}

UnsolvabilityManager &UnsolvabilityManager::getInstance() {
    static UnsolvabilityManager instance;
    return instance;
}

int UnsolvabilityManager::get_new_setid() {
    return setcount++;
}

std::string &UnsolvabilityManager::get_directory() {
    return directory;
}

int UnsolvabilityManager::get_emptysetid() {
    return emptysetid;
}
int UnsolvabilityManager::get_truesetid() {
    return truesetid;
}
int UnsolvabilityManager::get_goalsetid() {
    return goalsetid;
}
int UnsolvabilityManager::get_initsetid() {
    return initsetid;
}

void UnsolvabilityManager::dump_state(const GlobalState &state, std::ofstream &stream) {
    char c(0);
    std::vector<char> cvec;
    int count = 0;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        for(int j = 0; j < g_variable_domain[i]; ++j) {
            if((state[i] == j)) {
                c |= 1;
            }
            count++;
            if(count%8==0) {
                cvec.push_back(c);
                c = 0;
            } else {
                c <<= 1;
            }
        }
    }
    if(count%8 != 0) {
        c <<= 7-(count%8);
        cvec.push_back(c);
    }
    std::string s(cvec.begin(), cvec.end());
    stream << s;
}
