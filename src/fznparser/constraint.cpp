#include "constraint.hpp"

Constraint::Constraint() {}

Variable* Constraint::getVariable(
    const std::map<std::string, std::shared_ptr<Variable>>& variables,
    std::string name) {
  return variables.find(name)->second.get();
}
ConstraintItem::ConstraintItem() {}
ConstraintItem::ConstraintItem(std::string name,
                               std::vector<Expression> expressions,
                               std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}

IntDiv::IntDiv(ConstraintItem constraintItem) {
  _constraintItem = constraintItem;
  _name = constraintItem._name;
}
void IntDiv::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getVariable(variables, _constraintItem._expressions[0]._name);
  _b = getVariable(variables, _constraintItem._expressions[1]._name);
  _c = getVariable(variables, _constraintItem._expressions[2]._name);

  _next.push_back(_a);
  _b->_constraints.push_back(this);
  _c->_constraints.push_back(this);
}
void IntDiv::print() {
  std::cout << _name << std::endl;
  std::cout << _a->_name << std::endl;
  std::cout << _b->_name << std::endl;
  std::cout << _c->_name << std::endl;
}
std::vector<Node*> IntDiv::getNext() { return _next; }
