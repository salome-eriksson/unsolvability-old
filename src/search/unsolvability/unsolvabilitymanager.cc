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
    hex = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e' , 'f'};
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
    int c = 0;
    int count = 3;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        for(int j = 0; j < g_variable_domain[i]; ++j) {
            if((state[i] == j)) {
                c += (1 << count);
            }
            count--;
            if(count==-1) {
                stream << hex[c];
                c = 0;
                count = 3;
            }
        }
    }
    if(count != 3) {
        stream << hex[c];
    }
}
