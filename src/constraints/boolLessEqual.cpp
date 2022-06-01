#include "constraints/boolLessEqual.hpp"

#include "core/engine.hpp"

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
BoolLessEqual::BoolLessEqual(VarId violationId, VarId x, VarId y)
    : Constraint(violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BoolLessEqual::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void BoolLessEqual::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(_violationId, 0, 1, widenOnly);
}

void BoolLessEqual::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _violationId,
              static_cast<Int>((engine.value(ts, _x) == 0) &&
                               (engine.value(ts, _y) != 0)));
}

void BoolLessEqual::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId BoolLessEqual::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolLessEqual::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void BoolLessEqual::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
