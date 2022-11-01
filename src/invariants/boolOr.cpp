#include "invariants/boolOr.hpp"

/**
 * Invariant output = x \/ y. output does not violate if x or y does not
 * violate.
 * @param violationId id for the violationCount
 * @param x first violation variable
 * @param y second violation variable
 * @param output result
 */
BoolOr::BoolOr(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BoolOr::registerVars() {
  assert(_id != NULL_ID);
  _engine.registerInvariantInput(_id, _x, LocalId(0));
  _engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(_output);
}

void BoolOr::updateBounds(bool widenOnly) {
  _engine.updateBounds(
      _output, std::min(_engine.lowerBound(_x), _engine.lowerBound(_y)),
      std::min(_engine.upperBound(_x), _engine.upperBound(_y)), widenOnly);
}

void BoolOr::recompute(Timestamp ts) {
  updateValue(ts, _output,
              std::min(_engine.value(ts, _x), _engine.value(ts, _y)));
}

void BoolOr::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId BoolOr::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolOr::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
