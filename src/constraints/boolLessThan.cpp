#include "constraints/boolLessThan.hpp"

#include "core/engine.hpp"

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
BoolLessThan::BoolLessThan(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BoolLessThan::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void BoolLessThan::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(_violationId, 0, 1, widenOnly);
}

void BoolLessThan::recompute(Timestamp ts, Engine& engine) {
  updateValue(
      ts, engine, _violationId,
      static_cast<Int>(engine.value(ts, _x) == 0) + engine.value(ts, _y));
}

void BoolLessThan::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId BoolLessThan::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolLessThan::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void BoolLessThan::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
