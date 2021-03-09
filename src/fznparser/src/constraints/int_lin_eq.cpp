#include "int_lin_eq.hpp"

void IntLinEq::loadVariables(const VariableMap& variableMap) {
  _as = getArrayVariable(variableMap, 0);  // Parameter
  _bs = getArrayVariable(variableMap, 1);
  _c = getSingleVariable(variableMap, 2);  // Parameter
  for (auto variable : _bs->elements()) {
    _variables.push_back(variable);
  }
  // We potentially need to remove _bs from variables.
}
void IntLinEq::configureVariables() {
  for (int i = 0; i < _as->elements().size(); i++) {
    std::string coefficient = _as->elements()[i]->getName();
    if (coefficient == "1" || coefficient == "-1") {
      _bs->elements()[i]->addPotentialDefiner(this);
    }
  }
}
bool IntLinEq::canBeImplicit() {
  for (int i = 0; i < _as->elements().size(); i++) {
    std::string coefficient = _as->elements()[i]->getName();
    if (!(coefficient == "1" || coefficient == "-1")) {
      return false;
    }
    if (!_bs->elements()[i]->isDefinable()) {
      return false;
    }
  }
  return true;
}
void IntLinEq::imposeDomain(Variable* variable) {
  // TODO: Verify this whole function
  auto it = std::find(_variables.begin(), _variables.end(), variable);
  Int n = it - _variables.begin();
  Int outCo = stol(_as->elements()[n]->getName());
  Int lb = 0;
  Int ub = 0;
  for (int i = 0; i < _as->elements().size(); i++) {
    if (i == n) {
      continue;
    }
    Int co = stol(_as->elements()[i]->getName());
    co = -outCo * co;
    lb += co < 0 ? co * _variables[i]->upperBound()
                 : co * _variables[i]->lowerBound();
    ub += co < 0 ? co * _variables[i]->lowerBound()
                 : co * _variables[i]->upperBound();
  }
  _outputDomain->setLower(lb);
  _outputDomain->setUpper(ub);

  variable->imposeDomain(_outputDomain.get());

  for (auto node : variable->getNext()) {
    auto constraint = dynamic_cast<Constraint*>(node);
    constraint->refreshImpose();
  }
}
