#include "element.hpp"

// element(var int: idx,int: idxoffset,array [int] of var int: x,var int: c);
void Element::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 3));
  _values = getArrayVariable(variableMap, 2);
  for (auto v : _values->elements()) {
    _variables.push_back(v);
  }
}
void Element::configureVariables() {
  for (int i = 0; i <= 1; i++) {
    if (_variables[i]->isDefinable()) {
      _variables[i]->addPotentialDefiner(this);
    }
  }
}

std::vector<Node*> Element::getNext() {
  std::vector<Variable*> defines;
  std::vector<Node*> sorted;
  if (_allowDynamicCycles) {
    return sorted;
  }
  for (auto var : _defines) {
    defines.push_back(var);
  }
  std::sort(defines.begin(), defines.end(), Variable::compareDomain);
  for (auto var : defines) {
    sorted.push_back(var);
  }
  return sorted;
}

void Element::checkAnnotations(const VariableMap& variableMap) {
  if (_constraintBox.hasDefineAnnotation()) {
    _annotationTarget.emplace(getAnnotationVariable(variableMap));
  }
  if (_constraintBox.hasImplicitAnnotation()) {
    _shouldBeImplicit = true;
  }
  if (_constraintBox.hasIgnoreCycleAnnotation()) {
    _allowDynamicCycles = true;
  }
}
