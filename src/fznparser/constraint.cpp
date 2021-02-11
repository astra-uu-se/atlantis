#include "constraint.hpp"

/********************* Constraint Item *********************/
ConstraintItem::ConstraintItem() {}
ConstraintItem::ConstraintItem(std::string name,
                               std::vector<Expression> expressions,
                               std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}

/********************* Constraint **************************/
Constraint::Constraint() {}

Variable* Constraint::getVariable(
    const std::map<std::string, std::shared_ptr<Variable>>& variables,
    std::string name) {
  assert(variables.find(name) != variables.end());
  return variables.find(name)->second.get();
}

std::vector<Node*> Constraint::getNext() { return _next; }

void Constraint::defineVariable(Variable* variable) {
  _next.push_back(variable);
  variable->defineBy(this);
}

/********************* IntDiv ******************************/
IntDiv::IntDiv(ConstraintItem constraintItem) {
  _constraintItem = constraintItem;
  _name = constraintItem._name;
}
void IntDiv::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getVariable(variables, _constraintItem._expressions[0]._name);
  _b = getVariable(variables, _constraintItem._expressions[1]._name);
  _c = getVariable(variables, _constraintItem._expressions[2]._name);

  defineVariable(_a);
  _b->addConstraint(this);
  _c->addConstraint(this);
}

/********************* IntMax ******************************/
IntMax::IntMax(ConstraintItem constraintItem) {
  _constraintItem = constraintItem;
  _name = constraintItem._name;
}
void IntMax::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getVariable(variables, _constraintItem._expressions[0]._name);
  _b = getVariable(variables, _constraintItem._expressions[1]._name);
  _c = getVariable(variables, _constraintItem._expressions[2]._name);

  defineVariable(_c);
  _a->addConstraint(this);
  _b->addConstraint(this);
}

/********************* IntPlus ******************************/
IntPlus::IntPlus(ConstraintItem constraintItem) {
  _constraintItem = constraintItem;
  _name = constraintItem._name;
}
void IntPlus::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getVariable(variables, _constraintItem._expressions[0]._name);
  _b = getVariable(variables, _constraintItem._expressions[1]._name);
  _c = getVariable(variables, _constraintItem._expressions[2]._name);

  defineVariable(_a);
  _b->addConstraint(this);
  _c->addConstraint(this);
}
