#include "certificate.h"

class SearchCertificate : public Certificate {
private:
    BDD bdd_exp; //contains all expanded states
    BDD bdd_pr; //contains all pruned states
    std::vector<Certificate *> h_certificates;
    std::vector<State> dead_ends;
    std::vector<int> bdd_to_global_var_mapping;
    bool is_in_h_certificates(State& s);
public:
  SearchCertificate(Task *task, std::ifstream &in);
  virtual bool is_inductive();
  virtual bool contains_state(const State &state);
  virtual bool contains_goal();
};
