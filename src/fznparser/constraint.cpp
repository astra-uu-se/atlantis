#include "constraint.hpp"

ConstraintItem::ConstraintItem() {}
ConstraintItem::ConstraintItem(std::string name,
                               std::vector<Expression> expressions,
                               std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}

Constraint::Constraint(){};

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
