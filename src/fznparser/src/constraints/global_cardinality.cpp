#include "global_cardinality.hpp"

void GlobalCardinality::loadVariables(const VariableMap& variableMap) {
  _x = getArrayVariable(variableMap, 0);
  _cover = getArrayVariable(variableMap, 1);
  _counts = getArrayVariable(variableMap, 2);
  _variables.push_back(_x);       // x
  _variables.push_back(_counts);  // counts
}
void GlobalCardinality::configureVariables() {
  _variables[1]->addPotentialDefiner(this);
}
bool GlobalCardinality::canBeOneWay(Variable* variable) {
  if (variable != _variables[1]) {
    return false;
  }
  return _variables[1]->isDefinable();
}
bool GlobalCardinality::canBeImplicit() {
  for (auto variable : _variables) {
    if (variable->isDefined()) {
      return false;
    }
  }
  return true;
}
void GlobalCardinality::makeImplicit() {
  assert(canBeImplicit());
  for (auto variable : _variables) {
    _defines.insert(variable);
    auto v = dynamic_cast<ArrayVariable*>(variable);
    v->defineNotDefinedBy(this);
  }
  _implicit = true;
}
bool GlobalCardinality::split(int index, VariableMap& variables,
                              ConstraintMap& constraints) {
  assert(0 < index && index < _cover->length());

  cleanse();

  std::vector<Variable*> lCoverElements;
  std::vector<Variable*> lCountsElements;
  std::vector<Variable*> rCoverElements;
  std::vector<Variable*> rCountsElements;

  for (int i = 0; i < _cover->length(); i++) {
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
GlobalCardinality::GlobalCardinality(ArrayVariable* x, ArrayVariable* cover,
                                     ArrayVariable* counts) {
  _name = "global_cardinality";
  _x = x;
  _cover = cover;
  _counts = counts;
  _variables.push_back(_x);
  _variables.push_back(_counts);
  configureVariables();
}
void GlobalCardinality::imposeDomain() {}
