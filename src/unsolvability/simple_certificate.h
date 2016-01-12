#ifndef SIMPLE_CERTIFICATE_H
#define SIMPLE_CERTIFICATE_H

#include "certificate.h"

class SimpleCertificate : public Certificate {
private:
  BDD certificate;
public:
  SimpleCertificate(Task *task, std::ifstream &in);
  virtual bool contains_state(const Cube &state);
  virtual bool contains_goal();
  virtual bool is_inductive();
};

#endif /* SIMPLE_CERTIFICATE_H */
