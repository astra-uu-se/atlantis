#include "invariants/boolXor.hpp"

#include "core/engine.hpp"

/**
 * invariant output = ((x == 0) != (y == 0))
 * output does not violate if exactly one of x or y violates.
 * @param violationId id for the violationCount
 * @param x first violation variable
 * @param y second violation variable
 * @param output result
 */
BoolXor::BoolXor(VarId x, VarId y, VarId output)
    : Invariant(NULL_ID), _x(x), _y(y), _output(output) {
  _modifiedVars.reserve(1);
}

void BoolXor::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _output);
}

void BoolXor::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(_output, 0, 1, widenOnly);
}

void BoolXor::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _output,
              static_cast<Int>((engine.value(ts, _x) != 0) ==
                               (engine.value(ts, _y) != 0)));
}

void BoolXor::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId BoolXor::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolXor::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void BoolXor::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
