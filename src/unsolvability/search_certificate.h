#include "certificate.h"

class SearchCertificate : public Certificate {
private:
    BDD bdd_exp; //contains all expanded states
    BDD bdd_pr; // contains all pruned states
    std::vector<Certificate *> h_certificates;
    std::vector<int> bddvar_to_global_fact; // map (even) bdd vars to global fact index
    bool is_in_h_certificates(Cube& s);
    void print_statistics(double action_check, double pr_check, double hcert_check);
public:
  SearchCertificate(Task *task, std::ifstream &in);
  virtual bool is_inductive();
  virtual bool contains_cube(const Cube &cube);
};
