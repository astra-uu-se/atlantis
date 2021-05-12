#include "invariants/absDiff.hpp"

#include "core/engine.hpp"
extern Id NULL_ID;

AbsDiff::AbsDiff(VarId a, VarId b, VarId c)
    : Invariant(NULL_ID), _a(a), _b(b), _c(c) {
  _modifiedVars.reserve(1);
}

void AbsDiff::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _c);
  engine.registerInvariantParameter(_id, _a, 0);
  engine.registerInvariantParameter(_id, _b, 0);
}

void AbsDiff::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _c,
              std::abs(engine.getValue(ts, _a) - engine.getValue(ts, _b)));
}

void AbsDiff::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId AbsDiff::getNextParameter(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  switch (_state.getValue(ts)) {
    case 0:
      return _a;
    case 1:
      return _b;
    default:
      return NULL_ID;
  }
}

void AbsDiff::notifyCurrentParameterChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void AbsDiff::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
