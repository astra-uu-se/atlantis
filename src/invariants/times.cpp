#include "invariants/times.hpp"

#include "core/engine.hpp"

void Times::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _y);
  engine.registerInvariantInput(_id, _a, 0);
  engine.registerInvariantInput(_id, _b, 0);
}

void Times::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _y,
              engine.getValue(ts, _a) * engine.getValue(ts, _b));
}

VarId Times::getNextInput(Timestamp ts, Engine&) {
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

void Times::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Times::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void Times::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
