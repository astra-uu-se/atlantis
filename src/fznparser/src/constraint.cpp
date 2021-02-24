#include "constraint.hpp"

/********************* Constraint **************************/
Constraint::Constraint(ConstraintBox constraintBox) {
  _constraintBox = constraintBox;
  _name = constraintBox._name;
  _uniqueTarget = true;
  _implicit = false;
  _canBeImplicit = false;
  _hasDefineAnnotation = false;
}

Expression Constraint::getExpression(int n) {
  assert(n < _constraintBox._expressions.size());
  return _constraintBox._expressions[n];
}

std::set<Node*> Constraint::getNext() {
  std::set<Node*> defines;
  for (auto var : _defines) {
    defines.insert(var);
  }
  return defines;
}
std::string Constraint::getLabel() { return _name; }

bool Constraint::breakCycle() {
  makeSoft();
  return true;
}

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
Variable* Constraint::getAnnotationVariable(
    std::map<std::string, std::shared_ptr<Variable>> variables) {
  std::string name = _constraintBox.getAnnotationVariableName();

  assert(variables.find(name) != variables.end());
  return variables.find(name)->second.get();
}

void Constraint::defineVariable(Variable* variable) {
  _defines.insert(variable);
  variable->defineBy(this);
}
void Constraint::unDefineVariable(Variable* variable) {
  assert(variable->definedBy() == this);
  _defines.erase(variable);
  variable->removeDefinition();
}
void Constraint::addDependency(Variable* variable) {
  variable->addConstraint(this);
}
void Constraint::removeDependency(Variable* variable) {
  variable->removeConstraint(this);
}
void Constraint::clearVariables() {
  for (auto variable : _variables) {
    if (variable->definedBy() == this) {
      unDefineVariable(variable);
    }
    removeDependency(variable);
  }
  assert(_defines.empty());
}
bool Constraint::canDefineByAnnotation() {
  if (_hasDefineAnnotation) {
    if (_annotationDefineVariable->isDefinable()) {
      return true;
    }
  }
  return false;
}
void Constraint::checkAnnotations(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  if (_constraintBox.hasDefineAnnotation()) {
    _annotationDefineVariable = getAnnotationVariable(variables);
    _hasDefineAnnotation = true;
  }
}
void Constraint::makeOneWayByAnnotation() {
  defineVariable(_annotationDefineVariable);
  for (auto variable : _variables) {
    if (variable != _annotationDefineVariable) {
      addDependency(variable);
    }
  }
}
void Constraint::makeOneWay(Variable* variable) {
  assert(std::count(_variables.begin(), _variables.end(), variable));
  clearVariables();
  defineVariable(variable);
  for (auto v : _variables) {
    if (variable != v) {
      addDependency(v);
    }
  }
}
void Constraint::makeSoft() { clearVariables(); }

bool Constraint::canBeImplicit() { return _canBeImplicit; }
void Constraint::makeImplicit() {
  assert(_canBeImplicit);
  _implicit = true;
  for (auto variable : _variables) {
    defineVariable(variable);
  }
}

bool Constraint::definesNone() { return _defines.empty(); }
bool Constraint::uniqueTarget() { return _uniqueTarget; }
std::vector<Variable*> Constraint::variables() { return _variables; }

void Constraint::init(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  loadVariables(variables);
  configureVariables();
  checkAnnotations(variables);
}
/********************* ThreeSVarConstraint ******************************/
void ThreeSVarConstraint::loadVariables(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _variables.push_back(getSingleVariable(variables, 0));
  _variables.push_back(getSingleVariable(variables, 1));
  _variables.push_back(getSingleVariable(variables, 2));
}
void ThreeSVarConstraint::configureVariables() {
  _variables[2]->addPotentialDefiner(this);
}
/********************* IntDiv ******************************/
void IntDiv::configureVariables() { _variables[0]->addPotentialDefiner(this); }
/********************* IntPlus ******************************/
void IntPlus::configureVariables() {
  _variables[0]->addPotentialDefiner(this);
  _variables[1]->addPotentialDefiner(this);
  _variables[2]->addPotentialDefiner(this);
}
/********************* TwoSVarConstraint ******************************/
void TwoSVarConstraint::loadVariables(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _variables.push_back(getSingleVariable(variables, 0));
  _variables.push_back(getSingleVariable(variables, 1));
}
void TwoSVarConstraint::configureVariables() {
  _variables[1]->addPotentialDefiner(this);
}
/********************* GlobalCardinality ******************************/
void GlobalCardinality::loadVariables(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _cover = getArrayVariable(variables, 1);               // Parameter
  _variables.push_back(getArrayVariable(variables, 0));  // x
  _variables.push_back(getArrayVariable(variables, 2));  // counts
}
void GlobalCardinality::configureVariables() {
  _variables[2]->addPotentialDefiner(this);
}
/********************* IntLinEq ******************************/
void IntLinEq::loadVariables(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _as = getArrayVariable(variables, 0);  // Parameter
  _bs = getArrayVariable(variables, 1);
  _c = getSingleVariable(variables, 2);  // Parameter
  for (auto variable : _bs->elements()) {
    _variables.push_back(variable);
  }
  // We potentially need to remove _bs from variables.
}
void IntLinEq::configureVariables() {
  for (auto variable : _bs->elements()) {
    variable->addPotentialDefiner(this);  // Here we need to check if as[v] = 1
  }
}
/********************* AllDifferent ******************************/
void AllDifferent::loadVariables(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _x = getArrayVariable(variables, 0);  // Parameter
  for (auto variable : _x->elements()) {
    _variables.push_back(variable);
  }
}
void AllDifferent::configureVariables() {
  for (auto variable : _variables) {
    variable->addPotentialDefiner(this);
  }
}
bool AllDifferent::canBeImplicit() {
  for (auto variable : _variables) {
    if (variable->isDefined()) {
      return false;
    }
  }
  return true;
}
void AllDifferent::makeImplicit() {
  for (auto variable : _variables) {
    defineVariable(variable);
  }
}
/********************* Inverse ******************************/
void Inverse::loadVariables(
    const std::map<std::string, std::shared_ptr<Variable>>& variables) {
  _variables.push_back(getArrayVariable(variables, 0));
  _variables.push_back(getArrayVariable(variables, 1));
}
void Inverse::configureVariables() {
  _variables[0]->addPotentialDefiner(this);
  _variables[0]->addPotentialDefiner(this);
}
bool Inverse::canBeImplicit() {
  for (auto arrayVariable : _variables) {
    if (!arrayVariable->isDefinable()) {
      return false;
    }
  }
  return true;
}
void Inverse::makeImplicit() {
  for (auto variable : _variables) {
    defineVariable(variable);
  }
}
