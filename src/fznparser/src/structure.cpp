#include "structure.hpp"

#include <limits>

Int Domain::size() { return std::numeric_limits<Int>::max() - 1; }
Int BoolDomain::size() { return 2; }
Int BoolDomain::lowerBound() { return 0; }
Int BoolDomain::upperBound() { return 1; }
IntDomain::IntDomain() {
  _lb = std::numeric_limits<Int>::min() + 1;
  _ub = std::numeric_limits<Int>::max() - 1;
}
IntDomain::IntDomain(Int lb, Int ub) {
  _lb = lb;
  _ub = ub;
}
Int IntDomain::size() { return _ub - _lb + 1; }
Int IntDomain::lowerBound() { return _lb; }
Int IntDomain::upperBound() { return _ub; }
IntSetDomain::IntSetDomain(std::set<Int> set) { _set = set; }
Int IntSetDomain::size() { return _set.size(); }
Int IntSetDomain::lowerBound() {
  return *_set.lower_bound(std::numeric_limits<Int>::min());
}
Int IntSetDomain::upperBound() {
  return *_set.upper_bound(std::numeric_limits<Int>::max());
}
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
