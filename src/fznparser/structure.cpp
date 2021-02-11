#include "structure.hpp"

Domain::Domain() { _defined = false; }
bool Domain::defined() { return _defined; }
IntDomain::IntDomain() { _defined = false; }
IntDomain::IntDomain(int lb, int ub) {
  _defined = true;
  _lb = lb;
  _ub = ub;
}
IntDomain::IntDomain(std::set<int> set) {
  _defined = true;
  _set = set;
  _lb = *(set.begin());
  _ub = *(set.rbegin());
}
int IntDomain::getLb() { return _lb; }
int IntDomain::getUb() { return _ub; }
BoolDomain::BoolDomain() { _defined = false; }
int BoolDomain::getLb() { return false; }
int BoolDomain::getUb() { return true; }

Annotation::Annotation() {}
Annotation::Annotation(std::string name) { _name = name; }

Expression::Expression() {}
Expression::Expression(std::string name, bool isId) {
  _name = name;
  _isId = isId;
}

Variable::Variable(std::string name, std::shared_ptr<Domain> domain,
                   std::vector<Annotation> annotations) {
  _name = name;
  _domain = domain;
  _annotations = annotations;
  _isDefined = false;
}
void Variable::defineBy(Node* constraint) {
  _isDefined = true;
  _definedBy = constraint;
}
bool Variable::isDefined() {
  return _isDefined;
}
void Variable::addConstraint(Node* constraint) {
  _nextConstraints.push_back(constraint);
}
std::vector<Node*> Variable::getNext() { return _nextConstraints; }
