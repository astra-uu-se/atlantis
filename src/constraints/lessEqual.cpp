#include "constraints/lessEqual.hpp"

/**
 * Constraint x <= y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
LessEqual::LessEqual(Engine& engine, VarId violationId, VarId x, VarId y)
    : Constraint(engine, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void LessEqual::registerVars() {
  assert(_id != NULL_ID);
  _engine.registerInvariantInput(_id, _x, LocalId(0));
  _engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(_violationId);
}

void LessEqual::updateBounds(bool widenOnly) {
  _engine.updateBounds(
      _violationId,
      std::max(Int(0), _engine.lowerBound(_x) - _engine.upperBound(_y)),
      std::max(Int(0), _engine.upperBound(_x) - _engine.lowerBound(_y)),
      widenOnly);
}

void LessEqual::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              std::max(Int(0), _engine.value(ts, _x) - _engine.value(ts, _y)));
}

void LessEqual::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId LessEqual::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void LessEqual::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
