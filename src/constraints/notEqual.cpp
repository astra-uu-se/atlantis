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
  std::array<Int, 4> bounds{engine.lowerBound(_x), engine.upperBound(_x),
                            engine.lowerBound(_y), engine.upperBound(_y)};
  Int ub = 0;
  for (size_t i = 1; i < bounds.size(); ++i) {
    if (bounds[0] != bounds[i]) {
      ub = 1;
      break;
    }
  }
  engine.updateBounds(_violationId, 0, ub);
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
