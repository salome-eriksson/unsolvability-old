#ifndef SIMPLE_CERTIFICATE_H
#define SIMPLE_CERTIFICATE_H

#include "certificate.h"

class SimpleCertificate : public Certificate {
private:
  BDD certificate;
public:
  SimpleCertificate(Task *task, std::ifstream &in);
  virtual bool is_unsolvability_certificate();
  virtual bool is_inductive();
  virtual bool contains_state(const State &state);
  virtual bool contains_goal();
};

#endif /* SIMPLE_CERTIFICATE_H */
