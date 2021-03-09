#include "element.hpp"

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
