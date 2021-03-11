#include "int_lin_eq.hpp"

#include <unistd.h>

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
    if (!_bs->elements()[i]->isDefinable() || _bs->elements()[i]->isDefined()) {
      return false;  // TODO: Maybe allow things to be not definable
    }
  }
  return true;
}
bool IntLinEq::imposeDomain(Variable* variable) {
  std::cout << "====================" << std::endl;
  std::cout << "SETTING DOMAIN" << std::endl;
  std::cout << this->getName() << std::endl;
  std::cout << "VAR: " << (*_defines.begin())->getName() << std::endl;
  auto bounds = getBounds(variable);
  std::cout << "====================" << std::endl;
  _outputDomain->setLower(bounds.first);
  _outputDomain->setUpper(bounds.second);
  variable->imposeDomain(_outputDomain.get());
  std::cout << "SET VAR: " << (*_defines.begin())->getName() << std::endl;
  std::cout << "TO:" << std::endl;
  std::cout << "LB: " << bounds.first << std::endl;
  std::cout << "UB: " << bounds.second << std::endl;
  std::cout << "====================" << std::endl;

  return true;
}
bool IntLinEq::refreshDomain() {
  std::cout << "REFRESHING DOMAIN" << std::endl;
  std::cout << this->getName() << std::endl;
  std::cout << "VAR: " << (*_defines.begin())->getName() << std::endl;

  auto bounds = getBounds(*_defines.begin());
  std::cout << "====================" << std::endl;
  if (_outputDomain->lowerBound() != bounds.first ||
      _outputDomain->upperBound() != bounds.second) {
    _outputDomain->setLower(bounds.first);
    _outputDomain->setUpper(bounds.second);
    return true;
  }
  return false;
}
std::pair<Int, Int> IntLinEq::getBounds(Variable* variable) {
  for (auto v : _bs->elements()) {
    std::cout << v->getName() << std::endl;
    std::cout << "LB: " << v->lowerBound() << std::endl;
    std::cout << "UB: " << v->upperBound() << std::endl;
  }
  Int n = 0;
  for (auto v : _bs->elements()) {
    if (v == variable) break;
    n++;
  }
  assert(n < _bs->elements().size());

  Int outCo = stol(_as->elements()[n]->getName());
  Int lb = 0;
  Int ub = 0;
  for (int i = 0; i < _as->elements().size(); i++) {
    if (i == n) {
      continue;
    }
    Int co = stol(_as->elements()[i]->getName());
    co = -outCo * co;
    lb += (co < 0 ? co * _bs->elements()[i]->upperBound()
                  : co * _bs->elements()[i]->lowerBound());
    ub += (co < 0 ? co * _bs->elements()[i]->lowerBound()
                  : co * _bs->elements()[i]->upperBound());
  }
  return std::make_pair(lb, ub);
}
