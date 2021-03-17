#include "element.hpp"

void Element::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  // Not sure what the second argument does.
  _values = getArrayVariable(variableMap, 2);
  _variables.push_back(getSingleVariable(variableMap, 3));
  for (auto v : _values->elements()) {
    _variables.push_back(v);
  }
}
void Element::configureVariables() { _variables[0]->addPotentialDefiner(this); }

std::set<Node*> Element::getNext() {
  std::set<Node*> defines;
  bool b = false;
  if (_variables[0]->isDefined()) {
    b = dynamic_cast<Circuit*>(_variables[0]->definedBy());
  }
  if (_allowDynamicCycles && b) {
    return defines;
  }
  for (auto var : _defines) {
    defines.insert(var);
  }
  return defines;
}
