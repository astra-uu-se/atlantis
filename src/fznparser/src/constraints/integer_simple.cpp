#include "integer_simple.hpp"

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
// TODO: Does not correctly calculate domain
void IntDiv::configureVariables() { _variables[0]->addPotentialDefiner(this); }
bool IntDiv::imposeDomain(Variable* variable) {
  _outputDomain->setLower(_variables[1]->lowerBound());
  _outputDomain->setUpper(_variables[1]->upperBound());
  variable->imposeDomain(_outputDomain.get());
  return true;
}
bool IntDiv::refreshDomain() {
  if (_outputDomain->lowerBound() != _variables[1]->lowerBound() ||
      _outputDomain->upperBound() != _variables[1]->upperBound()) {
    return imposeDomain((*_defines.begin()));
  }
  return false;
}
/********************* IntPlus ******************************/
void IntPlus::configureVariables() {
  _variables[0]->addPotentialDefiner(this);
  _variables[1]->addPotentialDefiner(this);
  _variables[2]->addPotentialDefiner(this);
}
bool IntPlus::imposeDomain(Variable* variable) {
  auto bounds = getBounds(variable);
  _outputDomain->setLower(bounds.first);
  _outputDomain->setUpper(bounds.second);
  variable->imposeDomain(_outputDomain.get());
  return true;
}
bool IntPlus::refreshDomain() {
  auto bounds = getBounds(*_defines.begin());
  if (_outputDomain->lowerBound() != bounds.first ||
      _outputDomain->upperBound() != bounds.second) {
    _outputDomain->setLower(bounds.first);
    _outputDomain->setUpper(bounds.second);
    return true;
  }
  return false;
}

std::pair<Int, Int> IntPlus::getBounds(Variable* variable) {
  Int lb;
  Int ub;

  if (variable == _variables[0]) {  // a = c - b
    lb = (_variables[2]->lowerBound() - _variables[1]->upperBound());
    ub = (_variables[2]->upperBound() - _variables[1]->lowerBound());
  } else if (variable == _variables[1]) {  // b = c - a
    lb = (_variables[2]->lowerBound() - _variables[0]->upperBound());
    ub = (_variables[2]->upperBound() - _variables[0]->lowerBound());
  } else if (variable == _variables[2]) {  // c = a + b
    lb = (_variables[0]->lowerBound() + _variables[1]->lowerBound());
    ub = (_variables[0]->upperBound() + _variables[1]->upperBound());
  }

  return std::make_pair(lb, ub);
}

/********************* TwoSVarConstraint ******************************/
void TwoSVarConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
}
void TwoSVarConstraint::configureVariables() {
  _variables[1]->addPotentialDefiner(this);
}
