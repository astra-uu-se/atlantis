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
void SingleVariable::addConstraint(Node* constraint) {
  _nextConstraints.insert(constraint);
}
void SingleVariable::removeConstraint(Node* constraint) {
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
int SingleVariable::imposedDomainSize() { return _imposedDomain->size(); }
void SingleVariable::imposeDomain(Domain* domain) {
  _imposedDomain = domain;
  _hasImposedDomain = true;
}
int SingleVariable::domainSize() { return _domain->size(); }
int SingleVariable::lowerBound() { return _domain->lowerBound(); }
int SingleVariable::upperBound() { return _domain->upperBound(); }

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

void ArrayVariable::addConstraint(Node* constraint) {
  for (auto e : _elements) {
    e->addConstraint(constraint);
  }
}
void ArrayVariable::removeConstraint(Node* constraint) {
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

int ArrayVariable::domainSize() {
  int n = 0;
  for (auto variable : _elements) {
    n += variable->domainSize();
  }
  return n;
}
int ArrayVariable::imposedDomainSize() {
  int n = 0;
  for (auto variable : _elements) {
    n += variable->imposedDomainSize();
  }
  return n;
}
void ArrayVariable::imposeDomain(Domain* domain) {
  for (auto variable : _elements) {
    variable->imposeDomain(domain);
  }
}
int ArrayVariable::lowerBound() {
  int n = MAX_DOMAIN_SIZE;
  for (auto variable : _elements) {
    if (variable->lowerBound() < n) {
      n = variable->lowerBound();
    }
  }
  return n;
}
int ArrayVariable::upperBound() {
  int n = MIN_DOMAIN_SIZE;
  for (auto variable : _elements) {
    if (variable->upperBound() > n) {
      n = variable->upperBound();
    }
  }
  return n;
}
int ArrayVariable::length() { return _elements.size(); }
Variable* ArrayVariable::getElement(int n) { return _elements[n]; }

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
int Literal::lowerBound() { return std::stoi(_name); }
int Literal::upperBound() { return std::stoi(_name); }

/*******************PARAMETER****************************/
int Parameter::lowerBound() { return std::stoi(_value); }
int Parameter::upperBound() { return std::stoi(_value); }

/*******************VARIABLEMAP****************************/
Variable* VariableMap::add(std::shared_ptr<Variable> variable) {
  if (!exists(variable->getName())) {
    _variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
        variable->getName(), variable));
    _variableArray.push_back(variable.get());
  }
  return find(variable->getName());
}
Variable* VariableMap::find(const std::string name) const {
  assert(_variables.find(name) != _variables.end());
  return _variables.find(name)->second.get();
}
bool VariableMap::exists(std::string name) {
  return (_variables.find(name) != _variables.end());
}
std::vector<Variable*> VariableMap::getArray() { return _variableArray; }
