#include "variable.hpp"

#include <algorithm>
#include <string>

bool Variable::compareDomain(Variable* v1, Variable* v2) {
  if (v1->domainSize() == v2->domainSize()) {
    return v1->getName() > v2->getName();
  }
  return v1->domainSize() > v2->domainSize();
}

bool Variable::comparePotentialDefiners(Variable* v1, Variable* v2) {
  if (v1->potentialDefiners().size() == v2->potentialDefiners().size()) {
    return v1->getName() > v2->getName();
  }
  return v1->potentialDefiners().size() < v2->potentialDefiners().size();
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
  _orgPotentialDefiners.insert(constraint);
}
void Variable::removePotentialDefiner(Constraint* constraint) {
  assert(isDefinable());
  assert(_potentialDefiners.count(constraint));
  _potentialDefiners.erase(constraint);
}
bool Variable::hasPotentialDefiner(Constraint* constraint) {
  return _potentialDefiners.count(constraint);
}
std::vector<Constraint*> Variable::potentialDefiners() {
  std::vector<Constraint*> sorted;
  for (auto c : _potentialDefiners) {
    sorted.push_back(c);
  }
  std::sort(sorted.begin(), sorted.end(), Constraint::sort);
  return sorted;
}
bool Variable::noPotDef() {
  if (_orgPotentialDefiners.size() == 0) return true;
  for (auto constraint : _orgPotentialDefiners) {
    if (constraint->annotationTarget() &&
        constraint->annotationTarget().value() != this) {
      continue;
    } else {
      return false;
    }
  }
  return true;
}
bool Variable::hasDefinedAnn() {
  for (auto ann : _annotations) {
    if (ann._name == "is_defined_var") return true;
  }
  return false;
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
bool Variable::hasEnlargedDomain() {
  return hasImposedDomain() &&
         (_imposedDomain.value()->size() > _domain->size());
}
std::vector<Node*> Variable::getNext() {
  std::vector<Node*> next;
  for (auto c : getNextConstraint()) {
    next.push_back(static_cast<Node*>(c));
  }
  return next;
}
std::vector<Constraint*> Variable::getNextConstraint() {
  std::vector<Constraint*> sorted;
  for (auto n : _nextConstraints) {
    sorted.push_back(n);
  }
  std::sort(sorted.begin(), sorted.end(), Constraint::sort);
  return sorted;
}
void Variable::reset() {
  if (hasImposedDomain()) {
    unImposeDomain();
  }
  removeDefinition();
  _nextConstraints.clear();
  _potentialDefiners.clear();
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
bool ArrayVariable::contains(Variable* variable) {
  for (auto var : _elements) {
    if (var == variable) {
      return true;
    }
  }
  return false;
}
bool ArrayVariable::noneDefined() {
  for (auto var : _elements) {
    if (var->isDefined()) {
      return false;
    }
  }
  return true;
}
/*******************LITERAL****************************/
Int Literal::lowerBound() { return std::stoi(_valuename); }
Int Literal::upperBound() { return std::stoi(_valuename); }
Int Parameter::lowerBound() { return std::stoi(_value); }
Int Parameter::upperBound() { return std::stoi(_value); }
