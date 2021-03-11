#include "alldifferent.hpp"

void AllDifferent::loadVariables(const VariableMap& variableMap) {
  _x = getArrayVariable(variableMap, 0);  // Parameter
  for (auto variable : _x->elements()) {
    _variables.push_back(variable);
  }
}
void AllDifferent::configureVariables() {
  for (auto variable : _variables) {
    if (variable->isDefinable()) {
      variable->addPotentialDefiner(this);
    }
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
