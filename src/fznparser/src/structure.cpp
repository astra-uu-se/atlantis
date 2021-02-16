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
Annotation::Annotation(std::string name) { _name = name; }

Expression::Expression() {}
Expression::Expression(std::string name, bool isId) {
  _name = name;
  _isId = isId;
  _isArray = false;
}
Expression::Expression(std::string name, std::vector<Expression> elements, bool isId) {
  _name = name;
  _isId = isId;
  _elements = elements;
  _isArray = true;
}
std::string Expression::getName() { return _name; }

// void Item::addConstraint(Node* constraint) {}

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
std::vector<Node*> Variable::getNext() { return _nextConstraints; }

void SingleVariable::init(std::map<std::string, std::shared_ptr<Item>>& items) {}

void ArrayVariable::init(std::map<std::string, std::shared_ptr<Item>>& items) {
  for (auto e : _expressions) {
    auto name = e.getName();
    if (e.isId()) {
      assert(items.find(name) != items.end());
      _elements.push_back(items.find(name)->second.get());
    } else {
      if (items.find(name) != items.end()) {
        _elements.push_back(items.find(name)->second.get());
      } else {
        auto p = std::make_shared<Parameter>(name);
        items.insert(std::pair<std::string, std::shared_ptr<Item>>(
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
void Parameter::init(std::map<std::string, std::shared_ptr<Item>>& items) {}
std::vector<Node*> Parameter::getNext() {
  std::vector<Node*> s;
  return s;
}
std::string Parameter::getLabel() {
  return _value;}
std::string Parameter::getName() { return _value;}
bool Parameter::isDefinable() { return false;}
void Parameter::addConstraint(Node *constraint) {}
