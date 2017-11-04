#ifndef UNSOLVABILITYMANAGER_H
#define UNSOLVABILITYMANAGER_H

#include "../global_state.h"

#include <fstream>


class UnsolvabilityManager
{
private:
    int setcount;
    std::string directory;
    int emptysetid;
    int truesetid;
    int goalsetid;
    int initsetid;

    UnsolvabilityManager();
public:
    static UnsolvabilityManager &getInstance();
    int get_emptysetid();
    int get_truesetid();
    int get_goalsetid();
    int get_initsetid();
    int get_new_setid();
    std::string &get_directory();

    void dump_state(const GlobalState &state, std::ofstream &stream);

    UnsolvabilityManager(UnsolvabilityManager const&) = delete;
    void operator=(UnsolvabilityManager const&) = delete;
};

#endif // UNSOLVABILITYMANAGER_H
