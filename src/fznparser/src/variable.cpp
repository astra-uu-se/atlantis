#include "variable.hpp"
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
void SingleVariable::defineBy(Node* constraint) {
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
int SingleVariable::domainSize() { return _domain->size(); }
/*******************ARRAYVARIABLE****************************/
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
void ArrayVariable::defineBy(Node* constraint) {
  for (auto e : _elements) {
    e->defineBy(constraint);
  }
  _definedBy = constraint;
  _isDefined = true;
}
void ArrayVariable::removeDefinition() {
  for (auto e : _elements) {
    e->removeDefinition();
  }
}
void ArrayVariable::addPotentialDefiner(Constraint* constraint) {
  _potentialDefiners.insert(constraint);
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

/*******************PARAMETER****************************/
Literal::Literal(std::string value) {
  _name = value;
  _isDefined = false;
}
void Literal::init(VariableMap& variables) {}
std::set<Node*> Literal::getNext() {
  std::set<Node*> s;
  return s;
}
int Literal::domainSize() { return 0; }

/*******************VARIABLEMAP****************************/
void VariableMap::add(std::shared_ptr<Variable> variable) {
  _variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
      variable->getName(), variable));
  _variableArray.push_back(variable.get());
}
Variable* VariableMap::find(const std::string name) const {
  assert(_variables.find(name) != _variables.end());
  return _variables.find(name)->second.get();
}
bool VariableMap::exists(std::string name) {
  return (_variables.find(name) != _variables.end());
}
std::vector<Variable*> VariableMap::getArray() { return _variableArray; }
