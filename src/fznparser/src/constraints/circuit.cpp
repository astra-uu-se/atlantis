#include "circuit.hpp"

void Circuit::loadVariables(const VariableMap& variableMap) {
  // Not sure what the first argument does.
  _x = getArrayVariable(variableMap, 0);
  for (auto variable : _x->elements()) {
    _variables.push_back(variable);
  }
}
void Circuit::configureVariables() {
  for (auto variable : _variables) {
    if (variable->isDefinable()) {
      variable->addPotentialDefiner(this);
    }
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
