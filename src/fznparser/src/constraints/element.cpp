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
