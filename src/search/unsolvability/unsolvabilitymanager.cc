#include "unsolvabilitymanager.h"
#include "../globals.h"
#include "../global_operator.h"
#include "../utils/system.h"


UnsolvabilityManager::UnsolvabilityManager()
    : setcount(0), knowledgecount(0), directory(g_certificate_directory) {
    if(!directory.empty() && !(directory.back() == '/')) {
        directory += "/";
    }
    certstream.open(directory + "certificate.txt");

    emptysetid = setcount++;
    certstream << "e " << emptysetid << " c e\n";
    goalsetid = setcount++;
    certstream << "e " << goalsetid << " c g\n";
    initsetid = setcount++;
    certstream << "e " << initsetid << " c i\n";
    k_empty_dead = knowledgecount++;
    certstream << "k " << k_empty_dead << " d " << emptysetid << " d1\n";

    hex = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e' , 'f'};
}

UnsolvabilityManager &UnsolvabilityManager::getInstance() {
    static UnsolvabilityManager instance;
    return instance;
}

int UnsolvabilityManager::get_new_setid() {
    return setcount++;
}
int UnsolvabilityManager::get_new_knowledgeid() {
    return knowledgecount++;
}

int UnsolvabilityManager::get_emptysetid() {
    return emptysetid;
}
int UnsolvabilityManager::get_goalsetid() {
    return goalsetid;
}
int UnsolvabilityManager::get_initsetid() {
    return initsetid;
}
int UnsolvabilityManager::get_k_empty_dead() {
    return k_empty_dead;
}

std::ofstream &UnsolvabilityManager::get_stream() {
    return certstream;
}

std::string &UnsolvabilityManager::get_directory() {
    return directory;
}


void UnsolvabilityManager::dump_state(const GlobalState &state) {
    int c = 0;
    int count = 3;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        for(int j = 0; j < g_variable_domain[i]; ++j) {
            if((state[i] == j)) {
                c += (1 << count);
            }
            count--;
            if(count==-1) {
                certstream << hex[c];
                c = 0;
                count = 3;
            }
        }
    }
    if(count != 3) {
        certstream << hex[c];
    }
}

void UnsolvabilityManager::write_task_file() const{
    std::vector<std::vector<int>> fact_to_var(g_variable_domain.size(), std::vector<int>());
    int fact_amount = 0;
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        fact_to_var[i].resize(g_variable_domain[i]);
        for(int j = 0; j < g_variable_domain[i]; ++j) {
            fact_to_var[i][j] = fact_amount++;
        }
    }

    std::ofstream task_file;
    task_file.open("task.txt");

    task_file << "begin_atoms:" << fact_amount << "\n";
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
        for(size_t j = 0; j < g_fact_names[i].size(); ++j) {
            task_file << g_fact_names[i][j] << "\n";
        }
    }
    task_file << "end_atoms\n";

    task_file << "begin_init\n";
    for(size_t i = 0; i < g_fact_names.size(); ++i) {
        task_file << fact_to_var[i][g_initial_state_data[i]] << "\n";
    }
    task_file << "end_init\n";

    task_file << "begin_goal\n";
    for(size_t i = 0; i < g_goal.size(); ++i) {
        task_file << fact_to_var[g_goal[i].first][g_goal[i].second] << "\n";
    }
    task_file << "end_goal\n";


    task_file << "begin_actions:" << g_operators.size() << "\n";
    for(size_t op_index = 0;  op_index< g_operators.size(); ++op_index) {
        const GlobalOperator& op = g_operators[op_index];
        task_file << "begin_action\n"
                  << op.get_name() << "\n"
                  << "cost: "<< op.get_cost() <<"\n";
        const std::vector<GlobalCondition>& pre = op.get_preconditions();
        const std::vector<GlobalEffect>& post = op.get_effects();
        for(size_t i = 0; i < pre.size(); ++i) {
            task_file << "PRE:" << fact_to_var[pre[i].var][pre[i].val] << "\n";
        }
        for(size_t i = 0; i < post.size(); ++i) {
            if(!post[i].conditions.empty()) {
                std::cout << "CONDITIONAL EFFECTS, ABORT!";
                task_file.close();
                std::remove("task.txt");
                utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
            }
            task_file << "ADD:" << fact_to_var[post[i].var][post[i].val] << "\n";
            // all other facts from this FDR variable are set to false
            // TODO: can we make this more compact / smarter?
            for(int j = 0; j < g_variable_domain[post[i].var]; j++) {
                if(j == post[i].val) {
                    continue;
                }
                task_file << "DEL:" << fact_to_var[post[i].var][j] << "\n";
            }
        }
        task_file << "end_action\n";
    }
    task_file << "end_actions\n";
    task_file.close();
}
