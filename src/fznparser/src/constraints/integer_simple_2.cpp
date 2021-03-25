#include "integer_simple_2.hpp"

void TwoSVarConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
}
void TwoSVarConstraint::configureVariables() {
  if (_variables[1]->isDefinable()) {
    _variables[1]->addPotentialDefiner(this);
  }
}
