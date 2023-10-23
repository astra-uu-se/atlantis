#include "propagation/invariants/boolAnd.hpp"

namespace atlantis::propagation {

/**
 * Invariant output = x /\ y
 * output does not violate if x and y does not violate
 * @param violationId id for the violationCount
 * @param x first violation variable
 * @param y second violation variable
 * @param output the result
 */
BoolAnd::BoolAnd(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BoolAnd::registerVars() {
  assert(_id != NULL_ID);
  _engine.registerInvariantInput(_id, _x, LocalId(0));
  _engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(_output);
}

void BoolAnd::updateBounds(bool widenOnly) {
  _engine.updateBounds(
      _output, std::max(_engine.lowerBound(_x), _engine.lowerBound(_y)),
      std::max(_engine.upperBound(_x), _engine.upperBound(_y)), widenOnly);
}

void BoolAnd::recompute(Timestamp ts) {
  updateValue(ts, _output,
              std::max(_engine.value(ts, _x), _engine.value(ts, _y)));
}

void BoolAnd::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId BoolAnd::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolAnd::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation