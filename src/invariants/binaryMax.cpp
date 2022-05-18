#include "invariants/binaryMax.hpp"

#include "core/engine.hpp"

BinaryMax::BinaryMax(VarId a, VarId b, VarId y)
    : Invariant(NULL_ID), _a(a), _b(b), _y(y) {
  _modifiedVars.reserve(1);
}

void BinaryMax::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _a, 0);
  engine.registerInvariantInput(_id, _b, 0);
  registerDefinedVariable(engine, _y);
}

void BinaryMax::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(
      _y, std::max(engine.lowerBound(_a), engine.lowerBound(_b)),
      std::max(engine.upperBound(_a), engine.upperBound(_b)), widenOnly);
}

void BinaryMax::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _y,
              std::max(engine.value(ts, _a), engine.value(ts, _b)));
}

VarId BinaryMax::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _a;
    case 1:
      return _b;
    default:
      return NULL_ID;
  }
}

void BinaryMax::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void BinaryMax::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void BinaryMax::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
