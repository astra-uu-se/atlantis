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

  assert(engine.lowerBound(_b) != 0 || engine.upperBound(_b) != 0);
  if (engine.lowerBound(_b) <= 0 && 0 <= engine.upperBound(_b)) {
    _zeroReplacement = engine.upperBound(_b) >= 1 ? 1 : -1;
  }
}

void Mod::recompute(Timestamp ts, Engine& engine) {
  assert(_zeroReplacement != 0);
  const Int denominator = engine.value(ts, _b);
  updateValue(ts, engine, _y,
              engine.value(ts, _a) %
                  std::abs(denominator != 0 ? denominator : _zeroReplacement));
}

void Mod::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId Mod::nextInput(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  switch (_state.value(ts)) {
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
