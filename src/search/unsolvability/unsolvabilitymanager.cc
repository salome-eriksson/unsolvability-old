#include "unsolvabilitymanager.h"

#include "../utils/system.h"
#include "../task_proxy.h"


UnsolvabilityManager::UnsolvabilityManager(
        std::string directory, std::shared_ptr<AbstractTask> task)
    : task(task), task_proxy(*task), setcount(0), knowledgecount(0), directory(directory) {
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
    for(size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        for(int j = 0; j < task_proxy.get_variables()[i].get_domain_size(); ++j) {
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
    std::vector<std::vector<int>> fact_to_var(task_proxy.get_variables().size(), std::vector<int>());
    int fact_amount = 0;
    for(size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        fact_to_var[i].resize(task_proxy.get_variables()[i].get_domain_size());
        for(int j = 0; j < task_proxy.get_variables()[i].get_domain_size(); ++j) {
            fact_to_var[i][j] = fact_amount++;
        }
    }

    std::ofstream task_file;
    task_file.open("task.txt");

    task_file << "begin_atoms:" << fact_amount << "\n";
    for(size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        for(int j = 0; j < task_proxy.get_variables()[i].get_domain_size(); ++j) {
            task_file << task_proxy.get_variables()[i].get_fact(j).get_name() << "\n";
        }
    }
    task_file << "end_atoms\n";

    task_file << "begin_init\n";
    for(size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        task_file << fact_to_var[i][task_proxy.get_initial_state()[i].get_value()] << "\n";
    }
    task_file << "end_init\n";

    task_file << "begin_goal\n";
    for(size_t i = 0; i < task_proxy.get_goals().size(); ++i) {
        FactProxy f = task_proxy.get_goals()[i];
        task_file << fact_to_var[f.get_variable().get_id()][f.get_value()] << "\n";
    }
    task_file << "end_goal\n";


    task_file << "begin_actions:" << task_proxy.get_operators().size() << "\n";
    for(size_t op_index = 0;  op_index < task_proxy.get_operators().size(); ++op_index) {
        OperatorProxy op = task_proxy.get_operators()[op_index];

        task_file << "begin_action\n"
                  << op.get_name() << "\n"
                  << "cost: "<< op.get_cost() <<"\n";
        PreconditionsProxy pre = op.get_preconditions();
        EffectsProxy post = op.get_effects();

        for(size_t i = 0; i < pre.size(); ++i) {
            task_file << "PRE:" << fact_to_var[pre[i].get_variable().get_id()][pre[i].get_value()] << "\n";
        }
        for(size_t i = 0; i < post.size(); ++i) {
            if(!post[i].get_conditions().empty()) {
                std::cout << "CONDITIONAL EFFECTS, ABORT!";
                task_file.close();
                std::remove("task.txt");
                utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
            }
            FactProxy f = post[i].get_fact();
            task_file << "ADD:" << fact_to_var[f.get_variable().get_id()][f.get_value()] << "\n";
            // all other facts from this FDR variable are set to false
            // TODO: can we make this more compact / smarter?
            for(int j = 0; j < f.get_variable().get_domain_size(); j++) {
                if(j == f.get_value()) {
                    continue;
                }
                task_file << "DEL:" << fact_to_var[f.get_variable().get_id()][j] << "\n";
            }
        }
        task_file << "end_action\n";
    }
    task_file << "end_actions\n";
    task_file.close();
}
