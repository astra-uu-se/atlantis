#include "constraint.hpp"

#include <memory>
#include <string>

#include "structure.hpp"
#include "variable.hpp"

/********************* Constraint **************************/
Constraint::Constraint() {
  _uniqueTarget = true;
  _hasDefineAnnotation = false;
}
Constraint::Constraint(ConstraintBox constraintBox) {
  _constraintBox = constraintBox;
  _name = constraintBox._name;
  _uniqueTarget = true;
  _hasDefineAnnotation = false;
}
Expression Constraint::getExpression(Int n) {
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
std::string Constraint::getName() { return _name; }
std::string Constraint::getLabel() {
  std::string name = _name;
  if (_implicit) {
    name = "(implicit) " + name;
  } else if (_invariant) {
    name = "(invariant) " + name;
  } else {
    name = "(soft) " + name;
  }
  name = name + " #" + std::to_string(_constraintBox._id);
  return name;
}
bool Constraint::breakCycle() {
  makeSoft();
  return true;
}
SingleVariable* Constraint::getSingleVariable(const VariableMap& variableMap,
                                              Int n) {
  assert(!getExpression(n).isArray());
  std::string name = getExpression(n).getName();
  Variable* s = variableMap.find(name);
  return dynamic_cast<SingleVariable*>(s);
}
ArrayVariable* Constraint::getArrayVariable(const VariableMap& variableMap,
                                            Int n) {
  // assert(getExpression(n).isArray()); TODO: Figure out why not works
  std::string name = getExpression(n).getName();
  Variable* s = variableMap.find(name);
  return dynamic_cast<ArrayVariable*>(s);
}
Variable* Constraint::getAnnotationVariable(const VariableMap& variableMap) {
  std::string name = _constraintBox.getAnnotationVariableName();
  return variableMap.find(name);
}
void Constraint::defineVariable(Variable* variable) {
  _defines.insert(variable);
  variable->defineBy(this);
}
void Constraint::unDefineVariable(Variable* variable) {
  assert(variable->definedBy() == this);
  _defines.erase(variable);
  if (variable->hasImposedDomain()) {
    variable->unImposeDomain();
  }
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
void Constraint::checkAnnotations(const VariableMap& variableMap) {
  if (_constraintBox.hasDefineAnnotation()) {
    _annotationDefineVariable = getAnnotationVariable(variableMap);
    _hasDefineAnnotation = true;
  }
  if (_constraintBox.hasImplicitAnnotation()) {
    _shouldBeImplicit = true;
  }
}
void Constraint::makeOneWayByAnnotation() {
  defineVariable(_annotationDefineVariable);
  imposeAndPropagate(_annotationDefineVariable);
  for (auto variable : _variables) {
    if (variable != _annotationDefineVariable) {
      addDependency(variable);
    }
  }
  _invariant = true;
}
void Constraint::makeOneWay(Variable* variable) {
  assert(std::count(_variables.begin(), _variables.end(), variable));
  clearVariables();
  defineVariable(variable);
  imposeAndPropagate(variable);
  for (auto v : _variables) {
    if (variable != v) {
      addDependency(v);
    }
  }
  _invariant = true;
}
void Constraint::cleanse() {
  clearVariables();
  for (auto variable : _variables) {
    variable->removePotentialDefiner(this);
  }
}
void Constraint::makeSoft() {
  clearVariables();
  _invariant = false;
}
bool Constraint::canBeImplicit() { return false; }
bool Constraint::shouldBeImplicit() { return _shouldBeImplicit; }
void Constraint::makeImplicit() {
  assert(canBeImplicit());
  for (auto variable : _variables) {
    defineVariable(variable);
  }
  _implicit = true;
}
Int Constraint::defInVarCount() {
  Int defInVarCount = 0;
  for (auto v : _variables) {
    if (_defines.count(v) == 0) {
      defInVarCount += v->definedCount();
    }
  }
  return defInVarCount;
}
bool Constraint::definesNone() { return _defines.empty(); }
bool Constraint::uniqueTarget() { return _uniqueTarget; }
std::vector<Variable*> Constraint::variables() { return _variables; }
std::vector<Variable*> Constraint::variablesSorted() {
  std::vector<Variable*> next = variables();
  std::sort(next.begin(), next.end(), Variable::compareDomain);
  return next;
}
void Constraint::init(const VariableMap& variableMap) {
  loadVariables(variableMap);
  configureVariables();
  checkAnnotations(variableMap);
}
void Constraint::refreshImpose() {
  for (auto variable : _defines) {
    imposeDomain(variable);
  }
}
void Constraint::imposeNext(Variable* variable) {
  for (auto node : variable->getNext()) {
    auto constraint = dynamic_cast<Constraint*>(node);
    constraint->refreshImpose();
  }
}
void Constraint::imposeAndPropagate(Variable* variable) {
  imposeDomain(variable);
  imposeNext(variable);
}
