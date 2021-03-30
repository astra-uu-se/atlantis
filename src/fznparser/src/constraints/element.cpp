#include "element.hpp"

// element(var int: idx, array [int] of var int: x,var int: c);
void Element::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 2));
  _values = getArrayVariable(variableMap, 1);
}
void Element::configureVariables() {
  if (_variables[1]->isDefinable()) {
    _variables[1]->addPotentialDefiner(this);
  }
}
void VarElement::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 2));
  _values = getArrayVariable(variableMap, 1);
  for (auto v : _values->elements()) {
    _variables.push_back(v);
  }
}

bool VarElement::isIndexVar(Node* node) { return (node == _variables[0]); }
