#include "integer_simple_2.hpp"

void TwoSVarConstraint::loadVariables(const VariableMap& variableMap) {
  _variables.push_back(getSingleVariable(variableMap, 0));
  _variables.push_back(getSingleVariable(variableMap, 1));
}
void TwoSVarConstraint::configureVariables() {
  if (_variables[1]->isDefinable()) {
    _variables[1]->addPotentialDefiner(this);
  }
}
std::pair<Int, Int> IntAbs::calculateDomain(Variable* var) {
  Int lb = std::max(_variables[0]->lowerBound(), (Int)0);
  Int ub =
      std::max(-1 * _variables[0]->lowerBound(), _variables[0]->upperBound());
  return std::make_pair(lb, ub);
}
void IntEq::configureVariables() {
  if (_variables[1]->isDefinable()) {
    _variables[1]->addPotentialDefiner(this);
  }
  if (_variables[0]->isDefinable()) {
    _variables[0]->addPotentialDefiner(this);
  }
}
std::pair<Int, Int> IntEq::calculateDomain(Variable* var) {
  if (_variables[0] == var) {
    return std::make_pair(_variables[0]->lowerBound(),
                          _variables[0]->upperBound());
  } else {
    return std::make_pair(_variables[1]->lowerBound(),
                          _variables[1]->upperBound());
  }
}
