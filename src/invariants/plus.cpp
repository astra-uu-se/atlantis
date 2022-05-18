#include "invariants/plus.hpp"

#include "core/engine.hpp"

Plus::Plus(VarId a, VarId b, VarId y)
    : Invariant(NULL_ID), _a(a), _b(b), _y(y) {
  _modifiedVars.reserve(1);
}

void Plus::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _a, 0);
  engine.registerInvariantInput(_id, _b, 0);
  registerDefinedVariable(engine, _y);
}

void Plus::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(_y, engine.lowerBound(_a) + engine.lowerBound(_b),
                      engine.upperBound(_a) + engine.upperBound(_b), widenOnly);
}

void Plus::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _y, engine.value(ts, _a) + engine.value(ts, _b));
}

VarId Plus::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _a;
    case 1:
      return _b;
    default:
      return NULL_ID;
  }
}

void Plus::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Plus::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void Plus::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
