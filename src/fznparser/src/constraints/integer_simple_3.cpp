#include "integer_simple_3.hpp"

#include <algorithm>

void ThreeSVarConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
  _variables.push_back(getSingleVariable(variableMap, 2));
}
void ThreeSVarConstraint::configureVariables() {
  if (_variables[2]->isDefinable()) {
    _variables[2]->addPotentialDefiner(this);
  }
}
void IntDiv::configureVariables() {
  if (_variables[0]->isDefinable()) {
    _variables[0]->addPotentialDefiner(this);
  }
}
std::pair<Int, Int> IntDiv::calculateDomain(Variable* _) {
  std::vector<Int> exs;
  exs.push_back(_variables[1]->lowerBound() * _variables[2]->lowerBound());
  exs.push_back(_variables[1]->upperBound() * _variables[2]->upperBound());
  exs.push_back(_variables[1]->upperBound() * _variables[2]->lowerBound());
  exs.push_back(_variables[1]->lowerBound() * _variables[2]->upperBound());
  Int lb = *std::min_element(exs.begin(), exs.end());
  Int ub = *std::max_element(exs.begin(), exs.end());

  return std::make_pair(lb, ub);
}
void IntMax::configureVariables() {
  if (_variables[2]->isDefinable()) {
    _variables[2]->addPotentialDefiner(this);
  }
}
std::pair<Int, Int> IntMax::calculateDomain(Variable* _) {
  Int lb = std::max(_variables[0]->lowerBound(), _variables[1]->lowerBound());
  Int ub = std::max(_variables[0]->upperBound(), _variables[1]->upperBound());
  return std::make_pair(lb, ub);
}
void IntPlus::configureVariables() {
  for (int i = 0; i <= 2; i++) {
    if (_variables[i]->isDefinable()) {
      _variables[i]->addPotentialDefiner(this);
    }
  }
}
std::pair<Int, Int> IntPlus::calculateDomain(Variable* variable) {
  Int lb;
  Int ub;

  if (variable == _variables[0]) {  // a = c - b
    lb = (_variables[2]->lowerBound() - _variables[1]->upperBound());
    ub = (_variables[2]->upperBound() - _variables[1]->lowerBound());
  } else if (variable == _variables[1]) {  // b = c - a
    lb = (_variables[2]->lowerBound() - _variables[0]->upperBound());
    ub = (_variables[2]->upperBound() - _variables[0]->lowerBound());
  } else if (variable == _variables[2]) {  // c = a + b
    lb = (_variables[0]->lowerBound() + _variables[1]->lowerBound());
    ub = (_variables[0]->upperBound() + _variables[1]->upperBound());
  }

  return std::make_pair(lb, ub);
}
