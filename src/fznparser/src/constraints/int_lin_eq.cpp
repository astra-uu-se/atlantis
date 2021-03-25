#include "int_lin_eq.hpp"

#include <string>
#include <utility>

void IntLinEq::loadVariables(const VariableMap& variableMap) {
  _as = getArrayVariable(variableMap, 0);  // Parameter
  _bs = getArrayVariable(variableMap, 1);
  _c = getSingleVariable(variableMap, 2);  // Parameter
  for (auto variable : _bs->elements()) {
    _variables.push_back(variable);
  }
}
void IntLinEq::configureVariables() {
  for (int i = 0; i < _as->elements().size(); i++) {
    _asv.push_back(std::stol(_as->elements()[i]->getName()));
    if (_bs->elements()[i]->isDefinable()) {
      std::string coefficient = _as->elements()[i]->getName();
      if (coefficient == "1" || coefficient == "-1") {
        _bs->elements()[i]->addPotentialDefiner(this);
      }
    }
  }
}
bool IntLinEq::canBeImplicit() {
  for (int i = 0; i < _as->elements().size(); i++) {
    std::string coefficient = _as->elements()[i]->getName();
    if (!(coefficient == "1" || coefficient == "-1")) {
      return false;
    }
  }
  return true;
}
bool IntLinEq::imposeDomain(Variable* variable) {
  auto bounds = getBounds(variable);
  _outputDomain->setLower(bounds.first);
  _outputDomain->setUpper(bounds.second);
  variable->imposeDomain(_outputDomain.get());

  return true;
}
bool IntLinEq::refreshDomain() {
  auto bounds = getBounds(*_defines.begin());
  if (_outputDomain->lowerBound() != bounds.first ||
      _outputDomain->upperBound() != bounds.second) {
    _outputDomain->setLower(bounds.first);
    _outputDomain->setUpper(bounds.second);
    return true;
  }
  return false;
}
std::pair<Int, Int> IntLinEq::getBounds(Variable* variable) {
  Int lb = 0;
  Int ub = 0;
  Int i = 0;
  for (auto v : _bs->elements()) {
    if (v == variable) break;
    i++;
  }
  assert(i < _bs->elements().size());
  if (_asv[i] < 0) {
    for (int j = 0; j < _bs->elements().size(); j++) {
      if (j == i) continue;
      ub += maxDomain(_asv[j], _bs->elements()[j]);
      lb += minDomain(_asv[j], _bs->elements()[j]);
    }
  } else {
    for (int j = 0; j < _bs->elements().size(); j++) {
      if (j == i) continue;
      ub += minDomain(_asv[j], _bs->elements()[j]);
      lb += maxDomain(_asv[j], _bs->elements()[j]);
    }
  }
  lb = -1 * _asv[i] * (lb - std::stol(_c->getName()));
  ub = -1 * _asv[i] * (ub - std::stol(_c->getName()));
  return std::make_pair(lb, ub);
}
Int IntLinEq::maxDomain(int coef, Variable* variable) {
  if (coef <= 0) return coef * variable->lowerBound();
  return coef * variable->upperBound();
}
Int IntLinEq::minDomain(int coef, Variable* variable) {
  if (coef >= 0) return coef * variable->lowerBound();
  return coef * variable->upperBound();
}
