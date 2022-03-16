#include "constraints/equal.hpp"

#include "core/engine.hpp"

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
Equal::Equal(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Equal::init(Timestamp, Engine& engine) {
  assert(_id != NULL_ID);

  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void Equal::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _violationId,
              std::abs(engine.value(ts, _x) - engine.value(ts, _y)));
}

void Equal::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId Equal::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Equal::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Equal::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
