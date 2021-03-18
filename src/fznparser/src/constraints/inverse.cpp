#include "inverse.hpp"

#include <algorithm>

void Inverse::loadVariables(const VariableMap& variableMap) {
  _f = getArrayVariable(variableMap, 0);
  _invf = getArrayVariable(variableMap, 1);
  for (auto y : _f->elements()) {
    _variables.push_back(y);
  }
  for (auto x : _invf->elements()) {
    _variables.push_back(x);
  }
}
void Inverse::configureVariables() {
  for (auto var : _f->elements()) {
    if (var->isDefinable()) {
      var->addPotentialDefiner(this);
    }
  }
  for (auto var : _invf->elements()) {
    if (var->isDefinable()) {
      var->addPotentialDefiner(this);
    }
  }
}
bool Inverse::canDefine(Variable* variable) {
  assert(variable->isDefinable());
  return (
      (!_invariant && (_f->contains(variable) || _invf->contains(variable))) ||
      _out.value()->contains(variable));
}
void Inverse::define(Variable* variable) {
  assert(canDefine(variable));
  defineVariable(variable);
  if (!_invariant && _invf->contains(variable)) {
    for (auto v : _f->elements()) {
      addDependency(v);
    }
    _out.emplace(_invf);
  } else if (!_invariant && _f->contains(variable)) {
    for (auto v : _invf->elements()) {
      addDependency(v);
    }
    _out.emplace((_f));
  }
  _invariant = true;
  // imposeAndPropagate(variable);
}
void Inverse::unDefine(Variable* variable) {
  unDefineVariable(variable);
  if (_defines.empty()) {
    removeDependencies();
    _invariant = false;
    _out.reset();
  }
}
bool Inverse::canBeImplicit() { return true; }
