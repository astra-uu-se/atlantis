#include "array.hpp"

#include <algorithm>

void ArrayConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _x = getArrayVariable(variableMap, 1);
  for (auto var : _x->elements()) {
    _variables.push_back(var);
  }
}
void ArrayConstraint::configureVariables() {
  if (_variables[0]->isDefinable()) {
    _variables[0]->addPotentialDefiner(this);
  }
}

std::pair<Int, Int> ArrayIntMaximum::calculateDomain(Variable* _) {
  std::vector<Int> lbs;
  std::vector<Int> ubs;
  for (auto var : _x->elements()) {
    lbs.push_back(var->lowerBound());
    ubs.push_back(var->upperBound());
  }
  Int lb = *std::max_element(lbs.begin(), lbs.end());
  Int ub = *std::max_element(ubs.begin(), ubs.end());
  return std::make_pair(lb, ub);
}
std::pair<Int, Int> ArrayIntMinimum::calculateDomain(Variable* _) {
  std::vector<Int> lbs;
  std::vector<Int> ubs;
  for (auto var : _x->elements()) {
    lbs.push_back(var->lowerBound());
    ubs.push_back(var->upperBound());
  }
  Int lb = *std::min_element(lbs.begin(), lbs.end());
  Int ub = *std::min_element(ubs.begin(), ubs.end());
  return std::make_pair(lb, ub);
}
