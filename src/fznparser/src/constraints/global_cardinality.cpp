#include "global_cardinality.hpp"

#include <memory>

GlobalCardinality::GlobalCardinality(ArrayVariable* x, ArrayVariable* cover,
                                     ArrayVariable* counts) {
  // TODO: Add #id
  _name = "global_cardinality";
  _x = x;
  _cover = cover;
  _counts = counts;
  configureVariables();
}
void GlobalCardinality::loadVariables(const VariableMap& variableMap) {
  _x = getArrayVariable(variableMap, 0);
  _cover = getArrayVariable(variableMap, 1);
  _counts = getArrayVariable(variableMap, 2);
  // _variables.push_back(_x);       // x
  // _variables.push_back(_counts);  // counts
  initDomains();
}
void GlobalCardinality::configureVariables() {
  for (auto var : _counts->elements()) {
    if (var->isDefinable()) {
      var->addPotentialDefiner(this);
    }
  }
}
bool GlobalCardinality::canDefine(Variable* variable) {
  assert(variable->isDefinable());
  for (auto var : _counts->elements()) {
    if (var == variable) {
      return true;
    }
  }
  return false;
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
    makeSoft();
  }
}
bool GlobalCardinality::canBeImplicit() { return true; }
bool GlobalCardinality::allTargetsFree() {
  for (auto var : _counts->elements()) {
    if (var->isDefined()) {
      return false;
    }
  }
  for (auto var : _x->elements()) {
    if (var->isDefined()) {
      return false;
    }
  }
  return true;
}
void GlobalCardinality::makeImplicit() {
  clearVariables();
  for (auto var : _x->elements()) {
    redefineVariable(var);
  }
  for (auto var : _counts->elements()) {
    redefineVariable(var);
  }
  _implicit = true;
}
void GlobalCardinality::clearVariables() {
  for (auto variable : _counts->elements()) {
    if (variable->isDefined() && variable->definedBy() == this) {
      unDefineVariable(variable);
    }
    removeDependency(variable);
  }
  for (auto variable : _x->elements()) {
    if (variable->isDefined() && variable->definedBy() == this) {
      unDefineVariable(variable);
    }
    removeDependency(variable);
  }
  assert(_defines.empty());
}

bool GlobalCardinality::split(int index, VariableMap& variables,
                              ConstraintMap& constraints) {
  assert(0 < index && index < _cover->size());

  cleanse();

  std::vector<Variable*> lCoverElements;
  std::vector<Variable*> lCountsElements;
  std::vector<Variable*> rCoverElements;
  std::vector<Variable*> rCountsElements;

  for (int i = 0; i < _cover->size(); i++) {
    if (i < index) {
      lCoverElements.push_back(_cover->getElement(i));
      lCountsElements.push_back(_counts->getElement(i));
    } else {
      rCoverElements.push_back(_cover->getElement(i));
      rCountsElements.push_back(_counts->getElement(i));
    }
  }

  ArrayVariable* lCover = dynamic_cast<ArrayVariable*>(
      variables.add(std::make_shared<ArrayVariable>(lCoverElements)));
  ArrayVariable* lCounts = dynamic_cast<ArrayVariable*>(
      variables.add(std::make_shared<ArrayVariable>(lCountsElements)));
  ArrayVariable* rCover = dynamic_cast<ArrayVariable*>(
      variables.add(std::make_shared<ArrayVariable>(rCoverElements)));
  ArrayVariable* rCounts = dynamic_cast<ArrayVariable*>(
      variables.add(std::make_shared<ArrayVariable>(rCountsElements)));

  constraints.add(std::make_shared<GlobalCardinality>(_x, lCover, lCounts));
  constraints.add(std::make_shared<GlobalCardinality>(_x, rCover, rCounts));
  constraints.remove(this);

  return true;
}

void GlobalCardinality::imposeAndPropagate(Variable* _) {
  std::set<Constraint*> visited;
  visited.insert(this);
  for (int i = 0; i < _cover->elements().size(); i++) {
    auto bounds = getBounds(stol(_cover->elements()[i]->getName()));
    _outputDomains[i]->setLower(bounds.first);
    _outputDomains[i]->setUpper(bounds.second);
    _counts->elements()[i]->imposeDomain(_outputDomains[i].get());
    for (auto node : _counts->elements()[i]->getNext()) {
      auto constraint = dynamic_cast<Constraint*>(node);
      assert(constraint);
      constraint->refreshAndPropagate(visited);
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
    for (auto node : _counts->elements()[i]->getNext()) {
      auto constraint = dynamic_cast<Constraint*>(node);
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
