#include "invariants/mod.hpp"

#include "core/engine.hpp"

Mod::Mod(VarId a, VarId b, VarId y) : Invariant(NULL_ID), _a(a), _b(b), _y(y) {
  _modifiedVars.reserve(1);
}

void Mod::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _y);
  engine.registerInvariantInput(_id, _a, 0);
  engine.registerInvariantInput(_id, _b, 0);
}

void Mod::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _y,
              std::abs(engine.getValue(ts, _a) % engine.getValue(ts, _b)));
}

void Mod::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId Mod::getNextInput(Timestamp ts, Engine&) {
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

void Mod::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Mod::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
