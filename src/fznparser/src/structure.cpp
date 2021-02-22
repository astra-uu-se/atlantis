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
Annotation::Annotation(std::string name) {
  _name = name;

  if (name == "defines_var") {
    _definesVar = true;
  } else {
    _definesVar = false;
  }
}
bool Annotation::definesVar() { return _definesVar; }

Expression::Expression() {}
Expression::Expression(std::string name, bool isId) {
  _name = name;
  _isId = isId;
  _isArray = false;
}
Expression::Expression(std::string name, std::vector<Expression> elements,
                       bool isId) {
  _name = name;
  _isId = isId;
  _elements = elements;
  _isArray = true;
}
std::string Expression::getName() { return _name; }
