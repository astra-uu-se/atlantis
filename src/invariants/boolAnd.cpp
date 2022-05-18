#include "invariants/boolAnd.hpp"

#include "core/engine.hpp"

/**
 * Invariant output = x /\ y
 * output does not violate if x and y does not violate
 * @param violationId id for the violationCount
 * @param x first violation variable
 * @param y second violation variable
 * @param output the result
 */
BoolAnd::BoolAnd(VarId x, VarId y, VarId output)
    : Invariant(NULL_ID), _x(x), _y(y), _output(output) {
  _modifiedVars.reserve(1);
}

void BoolAnd::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _output);
}

void BoolAnd::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(
      _output, std::max(engine.lowerBound(_x), engine.lowerBound(_y)),
      std::max(engine.upperBound(_x), engine.upperBound(_y)), widenOnly);
}

void BoolAnd::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _output,
              std::max(engine.value(ts, _x), engine.value(ts, _y)));
}

void BoolAnd::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId BoolAnd::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolAnd::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void BoolAnd::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
