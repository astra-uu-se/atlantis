#include "constraint.hpp"

/********************* ConstraintBox *********************/
ConstraintBox::ConstraintBox() {}
ConstraintBox::ConstraintBox(std::string name,
                             std::vector<Expression> expressions,
                             std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}
void ConstraintBox::prepare(
    std::map<std::string, std::shared_ptr<Variable>>& variables) {
  for (auto e : _expressions) {
    if (variables.find(e.getName()) == variables.end()) {
      if (e.isArray()) {  // Create new entry for literal array.
        std::vector<Annotation> ann;
        auto av =
            std::make_shared<ArrayVariable>(e.getName(), ann, e._elements);
        variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
            av->getName(), static_cast<std::shared_ptr<Variable>>(av)));
      } else if (!e.isId()) {
        auto p = std::make_shared<Parameter>(e.getName());
        variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
            p->getName(), static_cast<std::shared_ptr<Variable>>(p)));
      }
    }
  }
}

/********************* Constraint **************************/
Constraint::Constraint() {}
Constraint::Constraint(ConstraintBox constraintBox) {
  _constraintBox = constraintBox;
  _name = constraintBox._name;
}

Expression Constraint::getExpression(int n) {
  assert(n < _constraintBox._expressions.size());
  return _constraintBox._expressions[n];
}

std::set<Node*> Constraint::getNext() { return _defines; }
std::string Constraint::getLabel() { return _name; }
void Constraint::tweak() {}

SingleVariable* Constraint::getSingleVariable(
    std::map<std::string, std::shared_ptr<Variable>> variables, int n) {
  std::string name = getExpression(n).getName();

  assert(variables.find(name) != variables.end());
  Variable* s = variables.find(name)->second.get();
  return dynamic_cast<SingleVariable*>(s);
}
ArrayVariable* Constraint::getArrayVariable(
    std::map<std::string, std::shared_ptr<Variable>> variables, int n) {
  std::string name = getExpression(n).getName();
  assert(variables.find(name) != variables.end());
  Variable* s = variables.find(name)->second.get();
  return dynamic_cast<ArrayVariable*>(s);
}

void Constraint::defineVariable(Variable* variable) {
  _defines.insert(variable);
  variable->defineBy(this);
}
void Constraint::unDefineVariable(Variable* variable) {
  _defines.erase(variable);
  variable->removeDefinition();
}
void Constraint::addDependency(Variable* variable) {
  variable->addConstraint(this);
}
void Constraint::removeDependency(Variable* variable) {
  variable->removeConstraint(this);
}

/********************* GlobalCardinality ******************************/
void GlobalCardinality::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _x = getArrayVariable(variables, 0);
  _cover = getSingleVariable(variables, 1);
  _counts = getArrayVariable(variables, 2);

  addDependency(_x);
  defineVariable(_counts);
  // Skriv om till count och en mindre version av sig själv?
  // Har vi en cykel och kan den undvikas?
}
/********************* IntDiv ******************************/
void IntDiv::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getSingleVariable(variables, 0);
  _b = getSingleVariable(variables, 1);
  _c = getSingleVariable(variables, 2);

  defineVariable(_a);
  addDependency(_b);
  addDependency(_c);
}
/********************* IntMax ******************************/
void IntMax::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getSingleVariable(variables, 0);
  _b = getSingleVariable(variables, 1);
  _c = getSingleVariable(variables, 2);

  defineVariable(_a);
  addDependency(_b);
  addDependency(_c);
}

/********************* IntPlus ******************************/
void IntPlus::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _a = getSingleVariable(variables, 0);
  _b = getSingleVariable(variables, 1);
  _c = getSingleVariable(variables, 2);

  defineVariable(_a);
  addDependency(_b);
  addDependency(_c);
}
void IntPlus::tweak() { defineArg(_state + 1 % 3); }
void IntPlus::defineArg(int n) {
  removeDependency(_a);
  removeDependency(_b);
  removeDependency(_c);
  unDefineVariable(_a);
  unDefineVariable(_b);
  unDefineVariable(_c);
  switch (n) {
    case 0:
      defineVariable(_a);
      addDependency(_b);
      addDependency(_c);
    case 1:
      defineVariable(_b);
      addDependency(_c);
      addDependency(_a);
    case 2:
      defineVariable(_c);
      addDependency(_a);
      addDependency(_b);
  }
}

/********************* IntLinEq ******************************/
void IntLinEq::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _as = getArrayVariable(variables, 0);
  _bs = getArrayVariable(variables, 1);
  _c = getSingleVariable(variables, 2);

  defineVariable(_bs);
}
