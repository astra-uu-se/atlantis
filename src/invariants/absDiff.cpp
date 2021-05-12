#include "invariants/absDiff.hpp"

#include "core/engine.hpp"
extern Id NULL_ID;

AbsDiff::AbsDiff(VarId a, VarId b, VarId c)
    : Invariant(NULL_ID), _a(a), _b(b), _c(c) {
  _modifiedVars.reserve(1);
}

void AbsDiff::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(e, _c);
  e.registerInvariantDependsOnVar(_id, _a, 0);
  e.registerInvariantDependsOnVar(_id, _b, 0);
}

void AbsDiff::recompute(Timestamp t, Engine& e) {
  updateValue(t, e, _c, std::abs(e.getValue(t, _a) - e.getValue(t, _b)));
}

void AbsDiff::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId AbsDiff::getNextDependency(Timestamp t, Engine&) {
  _state.incValue(t, 1);
  switch (_state.getValue(t)) {
    case 0:
      return _a;
    case 1:
      return _b;
    default:
      return NULL_ID;
  }
}

void AbsDiff::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void AbsDiff::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
