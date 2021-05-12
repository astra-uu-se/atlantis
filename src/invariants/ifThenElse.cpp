#include "invariants/ifThenElse.hpp"

IfThenElse::IfThenElse(VarId b, VarId x, VarId y, VarId z)
    : Invariant(NULL_ID), _b(b), _xy({x, y}), _z(z) {
  _modifiedVars.reserve(1);
}

void IfThenElse::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(e, _z);
  e.registerInvariantDependsOnVar(_id, _b, 0);
  e.registerInvariantDependsOnVar(_id, _xy[0], 0);
  e.registerInvariantDependsOnVar(_id, _xy[1], 0);
}

void IfThenElse::recompute(Timestamp t, Engine& e) {
  auto b = e.getValue(t, _b);
  updateValue(t, e, _z, e.getValue(t, _xy[1 - (b == 0)]));
}

void IfThenElse::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId IfThenElse::getNextDependency(Timestamp t, Engine& e) {
  _state.incValue(t, 1);
  auto state = _state.getValue(t);
  if (state == 0) {
    return _b;
  } else if (state == 1) {
    auto b = e.getValue(t, _b);
    return _xy[1 - (b == 0)];
  } else {
    return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void IfThenElse::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }