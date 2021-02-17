#include "variable.hpp"


Variable::Variable(std::string name, std::vector<Annotation> annotations) {
  _name = name;
  _annotations = annotations;
  _isDefined = false;
}

void Variable::defineBy(Node* constraint) {
  _isDefined = true;
  _definedBy = constraint;
}
bool SingleVariable::isDefined() { return _isDefined; }
void SingleVariable::addConstraint(Node* constraint) {
  _nextConstraints.push_back(constraint);
}
std::vector<Node*> SingleVariable::getNext() { return _nextConstraints; }

void SingleVariable::init(std::map<std::string, std::shared_ptr<Variable>>& variables) {}

void ArrayVariable::init(std::map<std::string, std::shared_ptr<Variable>>& variables) {
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
        variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
            name, p));
        ;
        _elements.push_back(p.get());
      }
    }
  }
}

std::vector<Node*> ArrayVariable::getNext() {
  std::vector<Node*> nodes;
  for (Node* e : _elements) {
    nodes.push_back(static_cast<Node*>(e));
  }
  return nodes;
}

void ArrayVariable::addConstraint(Node* constraint) {
  for (auto e : _elements) {
    e->addConstraint(constraint);
  }
}

Parameter::Parameter(std::string value) { _value = value; }
void Parameter::init(std::map<std::string, std::shared_ptr<Variable>>& variables) {}
std::vector<Node*> Parameter::getNext() {
  std::vector<Node*> s;
  return s;
}
std::string Parameter::getLabel() {
  return _value;}
std::string Parameter::getName() { return _value;}
bool Parameter::isDefinable() { return false;}
void Parameter::addConstraint(Node *constraint) {}
