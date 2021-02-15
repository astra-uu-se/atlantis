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
Constraint::Constraint(ConstraintItem constraintItem) {
  _constraintItem = constraintItem;
  _name = constraintItem._name;
}

Variable* Constraint::getVariable(
    const std::map<std::string, std::shared_ptr<Variable>>& variables,
    std::string name) {
  assert(variables.find(name) != variables.end());
  return variables.find(name)->second.get();
}
Expression Constraint::getExpression(int n) {
  assert(n < _constraintItem._expressions.size());
  return _constraintItem._expressions[n];
}
// TODO: Clean up for readability
// TODO: Consider what happens when this is called on non-array.
std::vector<Variable*> Constraint::getArrayVariable(
    const std::map<std::string, std::shared_ptr<Variable>>& variables, int n) {
  std::vector<Variable*> x;
  if (getExpression(n)._isId) {
    for (auto v : getVariable(variables, getExpression(n)._name)
                      ->_expression._expressions) {
      x.push_back(getVariable(variables, v._name));
    }
  } else {
    for (auto v : getExpression(n)._expressions) {
      x.push_back(getVariable(variables, v._name));
    }
  }
  return x;
}

std::vector<Node*> Constraint::getNext() { return _next; }

void Constraint::defineVariable(Variable* variable) {
  _next.push_back(variable);
  variable->defineBy(this);
}

/********************* IntDiv ******************************/
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
void IntPlus::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getVariable(variables, _constraintItem._expressions[0]._name);
  _b = getVariable(variables, _constraintItem._expressions[1]._name);
  _c = getVariable(variables, _constraintItem._expressions[2]._name);

  defineVariable(_a);
  _b->addConstraint(this);
  _c->addConstraint(this);
}

/********************* GlobalCardinality ******************************/
void GlobalCardinality::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  // [a, b, 3] -> [a, b]
  _x = getArrayVariable(variables, 0);
  _cover = getVariable(variables, getExpression(1)._name);
  _counts = getArrayVariable(variables, 2);

  for (auto v : _x) {
    v->addConstraint(this);
  }
  for (auto v : _counts) {
    defineVariable(v);
  }
  // Skriv om till count och en mindre version av sig själv?
  // Har vi en cykel och kan den undvikas?
}
