#include "inverse.hpp"
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
