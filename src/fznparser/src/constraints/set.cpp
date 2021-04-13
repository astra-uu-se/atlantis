#include "set.hpp"

void SetInReif::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 2));
}
void SetInReif::configureVariables() {
  if (_variables[1]->isDefinable()) {
    _variables[1]->addPotentialDefiner(this);
  }
}
