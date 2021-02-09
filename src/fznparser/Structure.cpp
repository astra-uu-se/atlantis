#include "Structure.h"

using namespace std;

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
}

Variable::Variable(std::string name, std::shared_ptr<Domain> domain,
                   vector<Annotation> annotations) {
  _name = name;
  _domain = domain;
  _annotations = annotations;
};

ConstraintItem::ConstraintItem() {}
ConstraintItem::ConstraintItem(std::string name,
                               std::vector<Expression> expressions,
                               std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}

Constraint::Constraint(){};
void Constraint::print() { std::cout << "Nothing to print\n"; }

IntDiv::IntDiv(ConstraintItem constraintItem) {
  _constraintItem = constraintItem;
  _name = constraintItem._name;
}
void IntDiv::init(std::map<std::string, std::shared_ptr<Variable>> variables) {
  _a = (variables.find(_constraintItem._expressions[0]._name))->second.get();
  _b = (variables.find(_constraintItem._expressions[1]._name))->second.get();
  _c = (variables.find(_constraintItem._expressions[2]._name))->second.get();
}
void IntDiv::print() {
  std::cout << _name << std::endl;
  std::cout << _a->_name << std::endl;
  std::cout << _b->_name << std::endl;
  std::cout << _c->_name << std::endl;
}

Model::Model(){};
void Model::init() {
  for (auto constraint : _constraints) {
    constraint->init(_variables);
  }
}
void Model::addVariable(std::shared_ptr<Variable> variable) {
  _variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
      variable->_name, variable));
}
void Model::addConstraint(ConstraintItem constraintItem) {
  if (constraintItem._name == "int_div") {
    _constraints.push_back(std::make_shared<IntDiv>(constraintItem));
  }
}
