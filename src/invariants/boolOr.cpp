#include "invariants/boolOr.hpp"

#include "core/engine.hpp"

/**
 * Invariant output = x \/ y. output does not violate if x or y does not
 * violate.
 * @param violationId id for the violationCount
 * @param x first violation variable
 * @param y second violation variable
 * @param output result
 */
BoolOr::BoolOr(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BoolOr::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _output);
}

void BoolOr::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(
      _output, std::min(engine.lowerBound(_x), engine.lowerBound(_y)),
      std::min(engine.upperBound(_x), engine.upperBound(_y)), widenOnly);
}

void BoolOr::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _output,
              std::min(engine.value(ts, _x), engine.value(ts, _y)));
}

void BoolOr::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId BoolOr::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolOr::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void BoolOr::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
