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
void IntDiv::configureVariables() { _variables[0]->addPotentialDefiner(this); }
void IntDiv::imposeDomain(Variable* variable) {
  _outputDomain->setLower(_variables[1]->lowerBound());
  _outputDomain->setUpper(_variables[1]->upperBound());
  variable->imposeDomain(_outputDomain.get());
}
/********************* IntPlus ******************************/
void IntPlus::configureVariables() {
  _variables[0]->addPotentialDefiner(this);
  _variables[1]->addPotentialDefiner(this);
  _variables[2]->addPotentialDefiner(this);
}
void IntPlus::imposeDomain(Variable* variable) {
  if (variable == _variables[0]) {  // a = c - b
    _outputDomain->setLower(_variables[2]->lowerBound() -
                            _variables[1]->upperBound());
    _outputDomain->setUpper(_variables[2]->upperBound() -
                            _variables[1]->lowerBound());
  } else if (variable == _variables[1]) {  // b = c - a
    _outputDomain->setLower(_variables[2]->lowerBound() -
                            _variables[0]->upperBound());
    _outputDomain->setUpper(_variables[2]->upperBound() -
                            _variables[0]->lowerBound());
  } else if (variable == _variables[2]) {  // c = a + b
    _outputDomain->setLower(_variables[0]->lowerBound() +
                            _variables[1]->lowerBound());
    _outputDomain->setUpper(_variables[0]->upperBound() +
                            _variables[1]->upperBound());
  }
  variable->imposeDomain(_outputDomain.get());

  for (auto node : variable->getNext()) {
    auto constraint = dynamic_cast<Constraint*>(node);
    constraint->refreshImpose();
  }
}
/********************* TwoSVarConstraint ******************************/
void TwoSVarConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
}
void TwoSVarConstraint::configureVariables() {
  _variables[1]->addPotentialDefiner(this);
}
