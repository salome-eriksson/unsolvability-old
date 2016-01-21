#ifndef STRONG_CONJUNCTIVE_CERTIFICATE_H
#define STRONG_CONJUNCTIVE_CERTIFICATE_H

#include "certificate.h"

class StrongConjunctiveCertificate : public Certificate {
private:
    std::vector<BDD> certificates;
public:
  StrongConjunctiveCertificate(Task *task, std::ifstream &in);
  virtual bool is_inductive();
  virtual bool contains_state(const Cube &state);
  virtual bool contains_goal();
};

#endif /* STRONG_CONJUNCTIVE_CERTIFICATE_H */
