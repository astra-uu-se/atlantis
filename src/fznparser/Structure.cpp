#include "Structure.h"

using namespace std;

Domain::Domain() {
  _undefined = true;
}
// int Domain::getLb() {
//   return -1;
// }
// int Domain::getUb() {
//   return -1;
// }
IntDomain::IntDomain() {
  _undefined = true;
}
IntDomain::IntDomain(int lb, int ub) {
  _undefined = false;
  _lb = lb;
  _ub = ub;
}
int IntDomain::getLb() {
  return _lb;
}
int IntDomain::getUb() {
  return _ub;
}
BoolDomain::BoolDomain() {
  _undefined = true;
}
int BoolDomain::getLb() {
  return false;
}
int BoolDomain::getUb() {
  return true;
}

Annotation::Annotation() {
 
}


Variable::Variable(string name, std::shared_ptr<Domain> domain, vector<Annotation> annotations) {
  _name = name;
  _domain = domain;
  _annotations = annotations;
};

Constraint::Constraint(string name) {
  _name = name;
};

Model::Model(std::vector<std::shared_ptr<Variable>> variables,
             std::vector<std::shared_ptr<Constraint>> constraints) {

  _variables = variables;
  _constraints = constraints;
};
