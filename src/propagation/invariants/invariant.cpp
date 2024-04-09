#include "atlantis/propagation/invariants/invariant.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

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
