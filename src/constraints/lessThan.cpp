#include "constraints/lessThan.hpp"

#include "core/engine.hpp"

/**
 * Constraint x < y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
LessThan::LessThan(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void LessThan::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void LessThan::updateBounds(Engine& engine) {
  engine.updateBounds(
      _violationId, 0,
      std::max(Int(0), engine.upperBound(_x) - engine.lowerBound(_y) + 1));
}

void LessThan::recompute(Timestamp ts, Engine& engine) {
  updateValue(
      ts, engine, _violationId,
      std::max(Int(0), engine.value(ts, _x) - engine.value(ts, _y) + 1));
}

void LessThan::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId LessThan::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void LessThan::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void LessThan::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
