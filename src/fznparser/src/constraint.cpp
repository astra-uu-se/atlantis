#include "constraint.hpp"

#include <memory>

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
std::string Constraint::getLabel() {
  if (_implicit) {
    return "(implicit) " + _name;
  }
  return _name;
}

bool Constraint::breakCycle() {
  makeSoft();
  return true;
}

SingleVariable* Constraint::getSingleVariable(const VariableMap& variableMap,
                                              int n) {
  assert(!getExpression(n).isArray());
  std::string name = getExpression(n).getName();
  Variable* s = variableMap.find(name);
  return dynamic_cast<SingleVariable*>(s);
}
ArrayVariable* Constraint::getArrayVariable(const VariableMap& variableMap,
                                            int n) {
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
  imposeDomain(variable);
  for (auto v : _variables) {
    if (variable != v) {
      addDependency(v);
    }
  }
}
void Constraint::cleanse() {
  clearVariables();
  for (auto variable : _variables) {
    variable->removePotentialDefiner(this);
  }
}
void Constraint::makeSoft() { clearVariables(); }

bool Constraint::canBeImplicit() { return false; }
void Constraint::makeImplicit() {
  assert(canBeImplicit());
  for (auto variable : _variables) {
    defineVariable(variable);
  }
  _implicit = true;
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

/********************* ThreeSVarConstraint ******************************/
void ThreeSVarConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
  _variables.push_back(getSingleVariable(variableMap, 2));
}
void ThreeSVarConstraint::configureVariables() {
  _variables[2]->addPotentialDefiner(this);
}
/********************* IntDiv ******************************/
void IntDiv::configureVariables() { _variables[0]->addPotentialDefiner(this); }
void IntDiv::imposeDomain(Variable* variable) {
  int lowerBound = _variables[1]->lowerBound();
  int upperBound = _variables[1]->upperBound();

  _imposedDomain = std::make_shared<IntDomain>(lowerBound, upperBound);
  variable->imposeDomain(_imposedDomain.get());
}
/********************* IntPlus ******************************/
void IntPlus::configureVariables() {
  _variables[0]->addPotentialDefiner(this);
  _variables[1]->addPotentialDefiner(this);
  _variables[2]->addPotentialDefiner(this);
}
/********************* TwoSVarConstraint ******************************/
void TwoSVarConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
}
void TwoSVarConstraint::configureVariables() {
  _variables[1]->addPotentialDefiner(this);
}
/********************* GlobalCardinality ******************************/
void GlobalCardinality::loadVariables(const VariableMap& variableMap) {
  _x = getArrayVariable(variableMap, 0);
  _cover = getArrayVariable(variableMap, 1);
  _counts = getArrayVariable(variableMap, 2);
  _variables.push_back(_x);       // x
  _variables.push_back(_counts);  // counts
}
void GlobalCardinality::configureVariables() {
  _variables[1]->addPotentialDefiner(this);
}
bool GlobalCardinality::canBeOneWay(Variable* variable) {
  if (variable != _variables[1]) {
    return false;
  }
  return _variables[1]->isDefinable();
}
bool GlobalCardinality::canBeImplicit() {
  for (auto variable : _variables) {
    if (variable->isDefined()) {
      return false;
    }
  }
  return true;
}
void GlobalCardinality::makeImplicit() {
  assert(canBeImplicit());
  for (auto variable : _variables) {
    _defines.insert(variable);
    auto v = dynamic_cast<ArrayVariable*>(variable);
    v->defineNotDefinedBy(this);
  }
  _implicit = true;
}

bool GlobalCardinality::split(
    int index, VariableMap& variables,
    std::vector<std::shared_ptr<Constraint>>& constraints) {
  cleanse();
  std::vector<Variable*> lCoverElements;
  std::vector<Variable*> lCountsElements;
  std::vector<Variable*> rCoverElements;
  std::vector<Variable*> rCountsElements;

  for (int i = 0; i < _cover->length(); i++) {
    if (i < index) {
      lCoverElements.push_back(_cover->getElement(i));
      lCountsElements.push_back(_counts->getElement(i));
    } else {
      rCoverElements.push_back(_cover->getElement(i));
      rCountsElements.push_back(_counts->getElement(i));
    }
  }

  auto leftCover = std::make_shared<ArrayVariable>(lCoverElements);
  auto leftCounts = std::make_shared<ArrayVariable>(lCountsElements);
  auto rightCover = std::make_shared<ArrayVariable>(rCoverElements);
  auto rightCounts = std::make_shared<ArrayVariable>(rCountsElements);

  std::string name = "test";
  std::vector<Annotation> annotations;
  std::vector<Expression> expressions;
  auto test = std::make_shared<ArrayVariable>(name, annotations, expressions);
  variables.add(test);
  variables.add(leftCover);
  variables.add(leftCounts);
  variables.add(rightCover);
  variables.add(rightCounts);

  auto leftGC = std::make_shared<GlobalCardinality>(_x, leftCover.get(),
                                                    leftCounts.get());
  auto rightGC = std::make_shared<GlobalCardinality>(_x, rightCover.get(),
                                                     rightCounts.get());
  constraints.push_back(leftGC);
  constraints.push_back(rightGC);

  return true;
}

GlobalCardinality::GlobalCardinality(ArrayVariable* x, ArrayVariable* cover,
                                     ArrayVariable* counts) {
  _name = "global_cardinality";
  _x = x;
  _cover = cover;
  _counts = counts;
  _variables.push_back(_x);
  _variables.push_back(_counts);
  configureVariables();
}
/********************* IntLinEq ******************************/
void IntLinEq::loadVariables(const VariableMap& variableMap) {
  _as = getArrayVariable(variableMap, 0);  // Parameter
  _bs = getArrayVariable(variableMap, 1);
  _c = getSingleVariable(variableMap, 2);  // Parameter
  for (auto variable : _bs->elements()) {
    _variables.push_back(variable);
  }
  // We potentially need to remove _bs from variables.
}
void IntLinEq::configureVariables() {
  for (int i = 0; i < _as->elements().size(); i++) {
    std::string coefficient = _as->elements()[i]->getName();
    if (coefficient == "1" || coefficient == "-1") {
      _bs->elements()[i]->addPotentialDefiner(this);
    }
  }
}
bool IntLinEq::canBeImplicit() {
  for (int i = 0; i < _as->elements().size(); i++) {
    std::string coefficient = _as->elements()[i]->getName();
    if (!(coefficient == "1" || coefficient == "-1")) {
      return false;
    }
    if (!_bs->elements()[i]->isDefinable()) {
      return false;
    }
  }
  return true;
}
/********************* AllDifferent ******************************/
void AllDifferent::loadVariables(const VariableMap& variableMap) {
  _x = getArrayVariable(variableMap, 0);  // Parameter
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
/********************* Inverse ******************************/
void Inverse::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getArrayVariable(variableMap, 0));
  _variables.push_back(getArrayVariable(variableMap, 1));
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

/********************* Element ******************************/
void Element::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  // Not sure what the second argument does.
  _variables.push_back(getArrayVariable(variableMap, 2));
  _variables.push_back(getSingleVariable(variableMap, 3));
}
void Element::configureVariables() {
  _variables[0]->addPotentialDefiner(this);
  _variables[1]->addPotentialDefiner(this);
}
/********************* Circuit ******************************/
void Circuit::loadVariables(const VariableMap& variableMap) {
  // Not sure what the first argument does.
  _x = getArrayVariable(variableMap, 1);
  for (auto variable : _x->elements()) {
    _variables.push_back(variable);
  }
}
void Circuit::configureVariables() {
  for (auto variable : _variables) {
    variable->addPotentialDefiner(this);
  }
}
bool Circuit::canBeImplicit() {
  for (auto variable : _variables) {
    if (variable->isDefined()) {
      return false;
    }
  }
  return true;
}
