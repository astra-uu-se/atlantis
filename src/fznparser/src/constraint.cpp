#include "constraint.hpp"

#include <algorithm>
#include <memory>
#include <string>

/********************* Constraint **************************/
Constraint::Constraint() { _uniqueTarget = true; }
Constraint::Constraint(ConstraintBox constraintBox) {
  _constraintBox = constraintBox;
  _name = constraintBox._name;
  _uniqueTarget = true;
}
Expression Constraint::getExpression(Int n) {
  assert(n < _constraintBox._expressions.size());
  return _constraintBox._expressions[n];
}
std::vector<Node*> Constraint::getNext() {
  std::vector<Node*> sorted;
  for (auto var : getNextVariable()) {
    sorted.push_back(static_cast<Node*>(var));
  }
  return sorted;
}
std::vector<Variable*> Constraint::getNextVariable() {
  std::vector<Variable*> defines;
  for (auto var : _defines) {
    defines.push_back(var);
  }
  std::sort(defines.begin(), defines.end(), Variable::compareDomain);
  return defines;
}
std::string Constraint::getName() {
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

bool Constraint::sort(Constraint* c1, Constraint* c2) {
  if (c1->_variables.size() == c2->_variables.size()) {
    return c1->getName() > c2->getName();
  }
  return c1->_variables.size() > c2->_variables.size();
}

bool Constraint::breakCycle(Variable* variable) {
  unDefine(variable);
  return true;
}
bool Constraint::breakCycleWithBan(Variable* variable) {
  variable->removePotentialDefiner(this);
  unDefine(variable);
  return true;
}
Variable* Constraint::getSingleVariable(const VariableMap& variableMap, Int n) {
  assert(!getExpression(n).isArray());
  std::string name = getExpression(n).getName();
  return variableMap.find(name);
}
ArrayVariable* Constraint::getArrayVariable(const VariableMap& variableMap,
                                            Int n) {
  // assert(getExpression(n).isArray()); TODO: Figure out why not works
  std::string name = getExpression(n).getName();
  return variableMap.findArray(name);
}
Variable* Constraint::getAnnotationVariable(const VariableMap& variableMap) {
  std::string name = _constraintBox.getAnnotationVariableName();
  return variableMap.find(name);  // TODO: can this be an array?
}

bool Constraint::allVariablesFree() {
  for (auto v : _variables) {
    if (v->isDefined()) {
      return false;
    }
  }
  return true;
}

void Constraint::redefineVariable(Variable* variable) {
  if (variable->isDefined()) {
    variable->definedBy()->unDefine(variable);
  }
  defineVariable(variable);
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
  variable->addNextConstraint(this);
}
void Constraint::removeDependency(Variable* variable) {
  variable->removeNextConstraint(this);
}
void Constraint::removeDependencies() {
  for (auto variable : _variables) {
    removeDependency(variable);
  }
}
void Constraint::removeDefinitions() {
  for (auto variable : _variables) {
    if (variable->isDefined() && variable->definedBy() == this) {
      unDefineVariable(variable);
    }
  }
}
void Constraint::clearVariables() {
  removeDependencies();
  removeDefinitions();
  assert(_defines.empty());
}
std::optional<Variable*> Constraint::annotationTarget() {
  return _annotationTarget;
}
void Constraint::checkAnnotations(const VariableMap& variableMap) {
  if (_constraintBox.hasDefineAnnotation()) {
    _annotationTarget.emplace(getAnnotationVariable(variableMap));
  }
  if (_constraintBox.hasImplicitAnnotation()) {
    _shouldBeImplicit = true;
  }
}
void Constraint::define(Variable* variable) {
  assert(std::count(_variables.begin(), _variables.end(), variable));
  clearVariables();
  defineVariable(variable);
  for (auto v : _variables) {
    if (variable != v) {
      addDependency(v);
    }
  }
  _invariant = true;
  imposeAndPropagate(variable);
}
void Constraint::makeSoft() {
  for (auto variable : _defines) {
    if (variable->hasImposedDomain()) {
      variable->unImposeDomain();
      for (auto constraint : variable->getNextConstraint()) {
        std::set<Constraint*> visited;
        constraint->refreshAndPropagate(visited);
      }
    }
  }
  clearVariables();
  _invariant = false;
  _implicit = false;
}
bool Constraint::shouldBeImplicit() { return _shouldBeImplicit; }
void Constraint::makeImplicit() {
  assert(canBeImplicit());
  clearVariables();
  for (auto variable : _variables) {
    if (variable->isDefinable()) {
      redefineVariable(variable);
    }
  }
  _implicit = true;
  _invariant = false;
}
Int Constraint::defInVarCount() {
  Int defInVarCount = 0;
  for (auto v : _variables) {
    if (_defines.count(v) == 0) {
      if (v->isDefined()) {
        defInVarCount++;
      }
    }
  }
  return defInVarCount;
}
bool Constraint::notFull() { return _defines.empty(); }
bool Constraint::uniqueTarget() { return _uniqueTarget; }
bool Constraint::onlyWrongAnnTarget() {
  for (auto var : _variables) {
    if (var->hasPotentialDefiner(this)) {
      if (var->hasDefinedAnn() && !annDefines(var)) {
        continue;
      } else {
        return false;
      }
    }
  }
  return true;
}
bool Constraint::annDefines(Variable* var) {
  return annotationTarget().has_value() && (annotationTarget().value() == var);
}
std::vector<Variable*> Constraint::variables() { return _variables; }
std::vector<Variable*> Constraint::variablesSorted() {
  std::vector<Variable*> next = variables();
  std::sort(next.begin(), next.end(), Variable::compareDomain);
  return next;
}
std::vector<Variable*> Constraint::dependencies() {
  assert(isInvariant() || isImplicit());
  std::vector<Variable*> depends;
  for (auto var : variablesSorted()) {
    if (!_defines.count(var)) {
      depends.push_back(var);
    }
  }
  return depends;
}
void Constraint::init(const VariableMap& variableMap) {
  _variables.clear();
  loadVariables(variableMap);
  configureVariables();
  checkAnnotations(variableMap);
}
void Constraint::refreshNext(std::set<Constraint*>& visited) {
  for (auto variable : getNextVariable()) {
    for (auto constraint : variable->getNextConstraint()) {
      constraint->refreshAndPropagate(visited);
    }
  }
}
void Constraint::imposeAndPropagate(Variable* variable) {
  std::set<Constraint*> visited;
  visited.insert(this);
  if (imposeDomain(variable)) {
    for (auto constraint : variable->getNextConstraint()) {
      assert(constraint);
      constraint->refreshAndPropagate(visited);
    }
  }
}
void Constraint::refreshAndPropagate(std::set<Constraint*>& visited) {
  if (visited.count(this)) return;
  if (isInvariant() && refreshDomain()) {
    visited.insert(this);
    refreshNext(visited);
  }
}
ConstraintBox::ConstraintBox(std::string name,
                             std::vector<Expression> expressions,
                             std::vector<Annotation> annotations) {
  _name = name;
  _expressions = expressions;
  _annotations = annotations;
}
void ConstraintBox::prepare(VariableMap& variables) {
  for (auto e : _expressions) {
    if (!variables.exists(e.getName())) {
      if (e.isArray()) {  // Create new entry for literal array.
        std::vector<Annotation> ann;
        auto av =
            std::make_shared<ArrayVariable>(e.getName(), ann, e._elements);
        variables.add(av);
      } else if (!e.isId()) {
        auto p = std::make_shared<Literal>(e.getName());
        variables.add(p);
      }
    }
  }
}
bool Constraint::defines(Variable* variable) {
  if (_defines.count(variable)) return true;
  return false;
}
bool ConstraintBox::hasIgnoreCycleAnnotation() {
  for (auto a : _annotations) {
    if (a._name == "ignore_cycle") {
      return true;
    }
  }
  return false;
}
bool ConstraintBox::hasImplicitAnnotation() {
  for (auto a : _annotations) {
    if (a._name == "implicit") {
      return true;
    }
  }
  return false;
}
bool ConstraintBox::hasDefineAnnotation() {
  for (auto a : _annotations) {
    if (a.definesVar()) {
      return true;
    }
  }
  return false;
}
std::string ConstraintBox::getAnnotationVariableName() {
  for (auto annotation : _annotations) {
    if (annotation._definesVar) {
      return annotation._variableName;
    }
  }
  return "";
}
void ConstraintBox::setId(Int id) { _id = id; }

bool SimpleConstraint::imposeDomain(Variable* var) {
  _outputDomain->setLower(calculateDomain(var).first);
  _outputDomain->setUpper(calculateDomain(var).second);
  var->imposeDomain(_outputDomain.get());
  return true;
}
bool SimpleConstraint::refreshDomain() {
  auto bounds = calculateDomain(*_defines.begin());
  if (_outputDomain->lowerBound() != bounds.first ||
      _outputDomain->upperBound() != bounds.second) {
    return imposeDomain((*_defines.begin()));
  }
  return false;
}
