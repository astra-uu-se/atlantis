#include "variable.hpp"

#include <string>

bool Variable::compareDomain(Variable* v1, Variable* v2) {
  return v1->domainSize() > v2->domainSize();
}
/*******************SINGLEVARIABLE****************************/
void Variable::addNextConstraint(Constraint* constraint) {
  _nextConstraints.insert(constraint);
}
void Variable::removeNextConstraint(Constraint* constraint) {
  _nextConstraints.erase(constraint);
}
void Variable::defineBy(Constraint* constraint) {
  assert(!_definedBy.has_value());
  _definedBy.emplace(constraint);
}
void Variable::removeDefinition() { _definedBy.reset(); }
void Variable::addPotentialDefiner(Constraint* constraint) {
  assert(isDefinable());
  _potentialDefiners.insert(constraint);
}
void Variable::removePotentialDefiner(Constraint* constraint) {
  assert(isDefinable());
  assert(_potentialDefiners.count(constraint));
  _potentialDefiners.erase(constraint);
}
void Variable::imposeDomain(Domain* domain) { _imposedDomain.emplace(domain); }
void Variable::unImposeDomain() {
  assert(hasImposedDomain());
  _imposedDomain.reset();
  std::set<Constraint*> visited;
  for (auto c : _nextConstraints) {
    c->refreshAndPropagate(visited);
  }
}
Int Variable::domainSize() {
  return hasImposedDomain() ? _imposedDomain.value()->size() : _domain->size();
}
Int Variable::lowerBound() {
  return hasImposedDomain() ? _imposedDomain.value()->lowerBound()
                            : _domain->lowerBound();
}
Int Variable::upperBound() {
  return hasImposedDomain() ? _imposedDomain.value()->upperBound()
                            : _domain->upperBound();
}
std::set<Node*> Variable::getNext() {
  std::set<Node*> next;
  for (auto c : _nextConstraints) {
    next.insert(static_cast<Node*>(c));
  }
  return next;
}
std::set<Constraint*> Variable::getNextConstraints() {
  return _nextConstraints;
}
/*******************ARRAYVARIABLE****************************/
ArrayVariable::ArrayVariable(std::vector<Variable*> elements) {
  std::string name = "[";

  for (auto e : elements) {
    name = name + e->getName();
    name = name + ",";
  }
  name = name.substr(0, name.size() - 1);
  name = name + "]";
  _elements = elements;
  _name = name;
}
void ArrayVariable::init(VariableMap& variables) {
  for (auto e : _expressions) {
    auto name = e.getName();
    if (e.isId()) {
      _elements.push_back(variables.find(name));
    } else {
      if (variables.exists(name)) {
        _elements.push_back(variables.find(name));
      } else {
        auto p = std::make_shared<Literal>(name);
        variables.add(p);
        _elements.push_back(p.get());
      }
    }
  }
}
std::vector<Variable*> ArrayVariable::elements() { return _elements; }
Variable* ArrayVariable::getElement(Int n) { return _elements[n]; }
/*******************LITERAL****************************/
Int Literal::lowerBound() { return std::stoi(_valuename); }
Int Literal::upperBound() { return std::stoi(_valuename); }
Int Parameter::lowerBound() { return std::stoi(_value); }
Int Parameter::upperBound() { return std::stoi(_value); }
