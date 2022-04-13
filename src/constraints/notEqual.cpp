#include "constraints/notEqual.hpp"

#include "core/engine.hpp"

/**
 * Constraint x != y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
NotEqual::NotEqual(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void NotEqual::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void NotEqual::updateBounds(Engine& engine) {
  const Int xLb = engine.lowerBound(_x);
  const Int xUb = engine.upperBound(_x);
  const Int yLb = engine.lowerBound(_y);
  const Int yUb = engine.upperBound(_y);
  if (xUb < yLb || yUb < xLb) {
    engine.updateBounds(_violationId, 0, 0);
    return;
  }

  for (const Int val : std::array<Int, 3>{xUb, yLb, yUb}) {
    if (xLb != val) {
      engine.updateBounds(_violationId, 0, 1);
      return;
    }
  }
  engine.updateBounds(_violationId, 1, 1);
}

void NotEqual::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _violationId,
              engine.value(ts, _x) == engine.value(ts, _y));
}

void NotEqual::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId NotEqual::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void NotEqual::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void NotEqual::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
