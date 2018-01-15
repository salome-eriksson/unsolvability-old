#ifndef UNSOLVABILITYMANAGER_H
#define UNSOLVABILITYMANAGER_H

#include "../global_state.h"

#include <fstream>


class UnsolvabilityManager
{
private:
    int setcount;
    int knowledgecount;

    int emptysetid;
    int goalsetid;
    int initsetid;
    int k_empty_dead;

    std::ofstream certstream;

    std::string directory;
    std::vector<char> hex;

    UnsolvabilityManager();
public:
    static UnsolvabilityManager &getInstance();

    int get_new_setid();
    int get_new_knowledgeid();

    int get_emptysetid();
    int get_goalsetid();
    int get_initsetid();
    int get_k_empty_dead();

    std::ofstream &get_stream();

    std::string &get_directory();

    void dump_state(const GlobalState &state);
    void write_task_file() const;

    UnsolvabilityManager(UnsolvabilityManager const&) = delete;
    void operator=(UnsolvabilityManager const&) = delete;
};

#endif // UNSOLVABILITYMANAGER_H
