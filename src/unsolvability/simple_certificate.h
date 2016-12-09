#ifndef SIMPLE_CERTIFICATE_H
#define SIMPLE_CERTIFICATE_H

#include "certificate.h"

// SimpleCertificate stores its bdd in its own variable additionaly
// to having it in the CertMap. This is mainly because this way it
// is simpler to handle and since it is only 1 object there is
// pretty much no overhead
class SimpleCertificate : public Certificate {
private:
    BDD bdd_certificate;
public:
  SimpleCertificate(Task *task, std::ifstream &in);
  virtual bool contains_state(const Cube &state);
  virtual bool contains_goal();
  virtual bool is_inductive();
};

#endif /* SIMPLE_CERTIFICATE_H */
