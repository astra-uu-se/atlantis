#include "atlantis/propagation/invariants/invariant.hpp"

#include "atlantis/propagation/solver.hpp"

namespace atlantis::propagation {

void Invariant::notify(LocalId id) { _modifiedVars.push(id); }

void Invariant::compute(Timestamp ts) {
  assert(_modifiedVars.size() > 0);
  assert(_primaryDefinedVar != NULL_ID);

  while (_modifiedVars.hasNext()) {
    // don't turn this into a for loop...
    this->notifyInputChanged(ts, _modifiedVars.pop());
  }
}

void Invariant::registerDefinedVar(VarId id) {
  if (_primaryDefinedVar == NULL_ID) {
    _primaryDefinedVar = id;
  } else {
    _definedVars.push_back(id);
  }
  _solver.registerDefinedVar(id, _id);
}

void Invariant::updateValue(Timestamp ts, VarId id, Int val) {
  _solver.updateValue(ts, id, val);
}

void Invariant::incValue(Timestamp ts, VarId id, Int val) {
  _solver.incValue(ts, id, val);
}
}  // namespace atlantis::propagation
