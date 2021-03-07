#include "structure.hpp"
#define MAX_DOMAIN_SIZE 2147483647
#define MIN_DOMAIN_SIZE -2147483647

int Domain::size() { return MAX_DOMAIN_SIZE; }
int BoolDomain::size() { return 2; }
int BoolDomain::lowerBound() { return 0; }
int BoolDomain::upperBound() { return 1; }
IntDomain::IntDomain() { _defined = false; }
IntDomain::IntDomain(int lb, int ub) {
  _defined = true;
  _lb = lb;
  _ub = ub;
}
int IntDomain::size() { return _defined ? _ub - _lb : MAX_DOMAIN_SIZE; }
int IntDomain::lowerBound() { return _lb; }
int IntDomain::upperBound() { return _ub; }
IntSetDomain::IntSetDomain(std::set<int> set) { _set = set; }
int IntSetDomain::size() { return _set.size(); }
int IntSetDomain::lowerBound() { return *_set.lower_bound(MIN_DOMAIN_SIZE); }
int IntSetDomain::upperBound() { return *_set.upper_bound(MAX_DOMAIN_SIZE); }
Annotation::Annotation() {}
Annotation::Annotation(std::string name, std::string variableName) {
  _name = name;

  if (name == "defines_var") {
    _definesVar = true;
    _variableName = variableName;
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
