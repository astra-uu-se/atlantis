#include "invariants/ifThenElse.hpp"

IfThenElse::IfThenElse(VarId b, VarId x, VarId y, VarId z)
    : Invariant(NULL_ID), _b(b), _xy({x, y}), _z(z) {
  _modifiedVars.reserve(1);
}

void IfThenElse::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _b, 0);
  engine.registerInvariantInput(_id, _xy[0], 0);
  engine.registerInvariantInput(_id, _xy[1], 0);
  registerDefinedVariable(engine, _z);
}

void IfThenElse::updateBounds(Engine& engine) {
  engine.updateBounds(
      _z, std::min(engine.lowerBound(_xy[0]), engine.lowerBound(_xy[1])),
      std::max(engine.upperBound(_xy[0]), engine.upperBound(_xy[1])));
}

void IfThenElse::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _z,
              engine.value(ts, _xy[1 - (engine.value(ts, _b) == 0)]));
}

void IfThenElse::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId IfThenElse::nextInput(Timestamp ts, Engine& engine) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _b;
    case 1:
      return _xy[1 - (engine.value(ts, _b) == 0)];
    default:
      return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void IfThenElse::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}