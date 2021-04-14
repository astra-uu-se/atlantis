#include "reified.hpp"

void ReifiedConstraint::configureVariables() {
  if (_variables[0]->isDefinable()) _variables[0]->addPotentialDefiner(this);
}

void ReifiedConstraintSingle::loadVariables(const VariableMap& variables) {
  _variables.push_back(getSingleVariable(variables, 2));
  _variables.push_back(getSingleVariable(variables, 0));
  _variables.push_back(getSingleVariable(variables, 1));
}
void ReifiedConstraintArray::loadVariables(const VariableMap& variables) {
  _variables.push_back(getSingleVariable(variables, 3));
  for (auto var : getArrayVariable(variables, 1)->elements()) {
    _variables.push_back(var);
  }
}
