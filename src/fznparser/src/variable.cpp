#include "variable.hpp"
Variable::Variable(std::string name, std::vector<Annotation> annotations) {
  _name = name;
  _annotations = annotations;
  _isDefined = false;
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
/*******************ARRAYVARIABLE****************************/
void ArrayVariable::init(
    std::map<std::string, std::shared_ptr<Variable>>& variables) {
  for (auto e : _expressions) {
    auto name = e.getName();
    if (e.isId()) {
      assert(variables.find(name) != variables.end());
      _elements.push_back(variables.find(name)->second.get());
    } else {
      if (variables.find(name) != variables.end()) {
        _elements.push_back(variables.find(name)->second.get());
      } else {
        auto p = std::make_shared<Parameter>(name);
        variables.insert(
            std::pair<std::string, std::shared_ptr<Variable>>(name, p));
        ;
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
}
void ArrayVariable::removeDefinition() {
  for (auto e : _elements) {
    e->removeDefinition();
  }
}

/*******************PARAMETER****************************/
Parameter::Parameter(std::string value) { _name = value; }
void Parameter::init(
    std::map<std::string, std::shared_ptr<Variable>>& variables) {}
std::set<Node*> Parameter::getNext() {
  std::set<Node*> s;
  return s;
}
