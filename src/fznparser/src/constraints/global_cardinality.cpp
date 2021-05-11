#include "global_cardinality.hpp"

#include <memory>

void GlobalCardinality::loadVariables(const VariableMap& variableMap) {
  _x = getArrayVariable(variableMap, 0);
  _cover = getArrayVariable(variableMap, 1);
  _counts = getArrayVariable(variableMap, 2);
  for (auto x : _x->elements()) {
    _variables.push_back(x);
  }
  for (auto x : _counts->elements()) {
    _variables.push_back(x);
  }
  initDomains();
}
void GlobalCardinality::configureVariables() {
  for (auto var : _counts->elements()) {
    if (var->isDefinable()) {
      var->addPotentialDefiner(this);
    }
  }
  for (auto var : _x->elements()) {
    if (var->isDefinable()) {
      var->addPotentialDefiner(this);
    }
  }
}
bool GlobalCardinality::canDefine(Variable* variable) {
  assert(variable->isDefinable());
  return _counts->contains(variable);
}
void GlobalCardinality::define(Variable* variable) {
  assert(canDefine(variable));
  defineVariable(variable);
  for (auto v : _x->elements()) {
    addDependency(v);
  }
  _invariant = true;
  imposeAndPropagate(variable);
}
void GlobalCardinality::unDefine(Variable* variable) {
  unDefineVariable(variable);
  if (_defines.empty()) {
    removeDependencies();
    _invariant = false;
  }
}
bool GlobalCardinality::canBeImplicit() { return true; }
void GlobalCardinality::makeImplicit() {
  clearVariables();
  for (auto var : _x->elements()) {
    if (var->isDefinable()) redefineVariable(var);
  }
  for (auto var : _counts->elements()) {
    if (var->isDefinable()) redefineVariable(var);
  }
  _implicit = true;
}

void GlobalCardinality::imposeAndPropagate(Variable* variable) {
  std::set<Constraint*> visited;
  visited.insert(this);

  for (int i = 0; i < _cover->elements().size(); i++) {
    if (variable == _counts->elements()[i]) {
      auto bounds = getBounds(stol(_cover->elements()[i]->getName()));
      _outputDomains[i]->setLower(bounds.first);
      _outputDomains[i]->setUpper(bounds.second);
      _counts->elements()[i]->imposeDomain(_outputDomains[i].get());
      for (auto constraint : _counts->elements()[i]->getNextConstraint()) {
        assert(constraint);
        constraint->refreshAndPropagate(visited);
      }
    }
  }
}

void GlobalCardinality::refreshAndPropagate(std::set<Constraint*>& visited) {
  if (visited.count(this)) return;
  if (!isInvariant()) return;
  for (int i = 0; i < _cover->elements().size(); i++) {
    auto bounds = getBounds(stol(_cover->elements()[i]->getName()));
    if (_outputDomains[i]->upperBound() == bounds.second) {
      continue;
    }
    _outputDomains[i]->setUpper(bounds.second);
    for (auto constraint : _counts->elements()[i]->getNextConstraint()) {
      assert(constraint);
      constraint->refreshAndPropagate(visited);
    }
  }
}
// TODO: A tighter lower bound can be found.
std::pair<Int, Int> GlobalCardinality::getBounds(Int n) {
  Int ub = 0;
  for (auto var : _x->elements()) {
    if (var->lowerBound() <= n && n <= var->upperBound()) {
      ub++;
    }
  }
  return std::make_pair(0, ub);
}
void GlobalCardinality::initDomains() {
  for (auto _ : _counts->elements())
    _outputDomains.push_back(std::make_shared<IntDomain>());
}
bool notFull() { return true; }
