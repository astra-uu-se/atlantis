#include "variable.hpp"

#include <string>

#define MAX_DOMAIN_SIZE 2147483647
#define MIN_DOMAIN_SIZE -2147483647

Variable::Variable(std::string name, std::vector<Annotation> annotations) {
  _name = name;
  _annotations = annotations;
  _isDefined = false;
}
bool Variable::compareDomain(Variable* v1, Variable* v2) {
  return v1->domainSize() > v2->domainSize();
}
/*******************SINGLEVARIABLE****************************/
void SingleVariable::addConstraint(Constraint* constraint) {
  _nextConstraints.insert(constraint);
}
void SingleVariable::removeConstraint(Constraint* constraint) {
  _nextConstraints.erase(constraint);
}
void SingleVariable::defineBy(Constraint* constraint) {
  assert(!_isDefined);
  _definedBy = constraint;
  _isDefined = true;
}
void SingleVariable::removeDefinition() {
  _definedBy = nullptr;
  _isDefined = false;
}
void SingleVariable::addPotentialDefiner(Constraint* constraint) {
  _potentialDefiners.insert(constraint);
}
void SingleVariable::removePotentialDefiner(Constraint* constraint) {
  _potentialDefiners.erase(constraint);
}
void SingleVariable::imposeDomain(Domain* domain) {
  _imposedDomain = domain;
  _hasImposedDomain = true;
}
void SingleVariable::unImposeDomain() {
  assert(_hasImposedDomain);
  _imposedDomain = nullptr;
  _hasImposedDomain = false;
}
Int SingleVariable::domainSize() {
  return hasImposedDomain() ? _imposedDomain->size() : _domain->size();
}
Int SingleVariable::lowerBound() {
  return hasImposedDomain() ? _imposedDomain->lowerBound()
                            : _domain->lowerBound();
}
Int SingleVariable::upperBound() {
  return hasImposedDomain() ? _imposedDomain->upperBound()
                            : _domain->upperBound();
}
std::set<Node*> SingleVariable::getNext() {
  std::set<Node*> next;
  for (auto c : _nextConstraints) {
    next.insert(static_cast<Node*>(c));
  }
  return next;
}
std::set<Constraint*> SingleVariable::getNextConstraints() {
  return _nextConstraints;
}
/*******************ARRAYVARIABLE****************************/
ArrayVariable::ArrayVariable(std::vector<Variable*> elements) {
  std::string name = "[";

  for (auto e : elements) {
    name = name + e->getLabel();
    name = name + ",";
  }
  name = name.substr(0, name.size() - 1);
  name = name + "]";
  _elements = elements;
  _name = name;
  _isDefined = false;
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
std::set<Node*> ArrayVariable::getNext() {
  std::set<Node*> nodes;
  for (Node* e : _elements) {
    nodes.insert(static_cast<Node*>(e));
  }
  return nodes;
}
std::set<Constraint*> ArrayVariable::getNextConstraints() {
  std::set<Constraint*> next;
  for (auto e : elements()) {
    for (auto c : e->getNextConstraints()) {
      next.insert(c);
    }
  }
  return next;
}
void ArrayVariable::addConstraint(Constraint* constraint) {
  for (auto e : _elements) {
    e->addConstraint(constraint);
  }
}
void ArrayVariable::removeConstraint(Constraint* constraint) {
  for (auto e : _elements) {
    e->removeConstraint(constraint);
  }
}
void ArrayVariable::defineBy(Constraint* constraint) {
  for (auto e : _elements) {
    e->defineBy(constraint);
  }
  _definedBy = constraint;
  _isDefined = true;
}
void ArrayVariable::defineNotDefinedBy(Constraint* constraint) {
  for (auto e : _elements) {
    if (!e->isDefined()) {
      e->defineBy(constraint);
    }
  }
  _definedBy = constraint;
  _isDefined = true;
}
void ArrayVariable::removeDefinition() {
  for (auto e : _elements) {
    e->removeDefinition();
  }
  _isDefined = false;
}
void ArrayVariable::addPotentialDefiner(Constraint* constraint) {
  _potentialDefiners.insert(constraint);
}
void ArrayVariable::removePotentialDefiner(Constraint* constraint) {
  _potentialDefiners.erase(constraint);
}
std::vector<Variable*> ArrayVariable::elements() { return _elements; }
bool ArrayVariable::isDefinable() {
  if (_isDefined) return false;
  for (auto e : _elements) {
    if (!e->isDefinable()) {
      return false;
    }
  }
  return true;
}
std::string ArrayVariable::getLabel() { return "[array] " + _name; }
Int ArrayVariable::domainSize() {
  Int n = 0;
  for (auto variable : _elements) {
    n += variable->domainSize();
  }
  return n;
}
void ArrayVariable::imposeDomain(Domain* domain) {
  for (auto variable : _elements) {
    variable->imposeDomain(domain);
  }
}
void ArrayVariable::unImposeDomain() {
  for (auto variable : _elements) {
    variable->unImposeDomain();
  }
}
Int ArrayVariable::lowerBound() {
  Int n = std::numeric_limits<Int>::max();
  for (auto variable : _elements) {
    if (variable->lowerBound() < n) {
      n = variable->lowerBound();
    }
  }
  return n;
}
Int ArrayVariable::upperBound() {
  Int n = std::numeric_limits<Int>::max();
  for (auto variable : _elements) {
    if (variable->upperBound() > n) {
      n = variable->upperBound();
    }
  }
  return n;
}
Int ArrayVariable::length() { return _elements.size(); }
Variable* ArrayVariable::getElement(Int n) { return _elements[n]; }
Int ArrayVariable::definedCount() {
  Int c = 0;
  for (auto e : elements()) {
    c += e->definedCount();
  }
  return c;
}
/*******************LITERAL****************************/
Literal::Literal(std::string value) {
  _name = value;
  _isDefined = false;
}
void Literal::init(VariableMap& variables) {}
std::set<Node*> Literal::getNext() {
  std::set<Node*> s;
  return s;
}
std::set<Constraint*> Literal::getNextConstraints() {
  std::set<Constraint*> s;
  return s;
}
Int Literal::lowerBound() { return std::stoi(_name); }
Int Literal::upperBound() { return std::stoi(_name); }
/*******************PARAMETER****************************/
Int Parameter::lowerBound() { return std::stoi(_value); }
Int Parameter::upperBound() { return std::stoi(_value); }
