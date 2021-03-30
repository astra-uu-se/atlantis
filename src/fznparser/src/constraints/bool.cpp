#include "bool.hpp"

#include <algorithm>

void BoolArrayConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 1));
  _as = getArrayVariable(variableMap, 0);
  for (auto var : _as->elements()) {
    _variables.push_back(var);
  }
}
void BoolArrayConstraint::configureVariables() {
  if (_variables[0]->isDefinable()) {
    _variables[0]->addPotentialDefiner(this);
  }
}
void BoolOneToOne::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
}
void BoolOneToOne::configureVariables() {
  if (_variables[0]->isDefinable()) {
    _variables[0]->addPotentialDefiner(this);
  }
  if (_variables[1]->isDefinable()) {
    _variables[1]->addPotentialDefiner(this);
  }
}
void BoolThreeVar::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
  _variables.push_back(getSingleVariable(variableMap, 2));
}
void BoolThreeVar::configureVariables() {
  if (_variables[2]->isDefinable()) {
    _variables[2]->addPotentialDefiner(this);
  }
}
void BoolLinEq::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 2));
  _bs = getArrayVariable(variableMap, 1);
  _as = getArrayVariable(variableMap, 0);
  for (auto var : _bs->elements()) {
    _variables.push_back(var);
  }
}
void BoolLinEq::configureVariables() {
  if (_variables[2]->isDefinable()) {
    _variables[2]->addPotentialDefiner(this);
  }
}

std::pair<Int, Int> BoolLinEq::calculateDomain(Variable* variable) {
  Int lb = 0;
  Int ub = 0;
  for (auto par : _as->elements()) {
    lb = std::min(lb, lb + std::stol(par->getName()));
    ub = std::max(ub, ub + std::stol(par->getName()));
  }
  return std::make_pair(lb, ub);
}
