#ifndef TASK_H
#define TASK_H

#include <vector>
#include <string>

typedef std::vector<bool> State;

struct Action {
  std::vector<int> pre;
  std::vector<int> add;
  std::vector<int> del;
};

class Task {
private:
  std::vector<std::string> fact_names;
  std::vector<Action> actions;
  State initial_state;
  std::vector<int> goal;
public:
  Task(std::string file);
  const std::vector<std::string>& get_fact_names();
  const std::string& get_fact(int n);
  int find_fact(std::string& fact_name);
  const Action& get_action(int n);
  int get_number_of_actions();
  int get_number_of_facts();
  const State& get_initial_state();
  const std::vector<int>& get_goal();
};

#endif /* TASK_H */
