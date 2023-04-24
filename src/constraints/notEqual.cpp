#include "constraints/notEqual.hpp"

/**
 * Constraint x != y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
NotEqual::NotEqual(Engine& engine, VarId violationId, VarId x, VarId y)
    : Constraint(engine, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void NotEqual::registerVars() {
  assert(_id != NULL_ID);
  _engine.registerInvariantInput(_id, _x, LocalId(0));
  _engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(_violationId);
}

void NotEqual::updateBounds(bool widenOnly) {
  const Int xLb = _engine.lowerBound(_x);
  const Int xUb = _engine.upperBound(_x);
  const Int yLb = _engine.lowerBound(_y);
  const Int yUb = _engine.upperBound(_y);
  if (xUb < yLb || yUb < xLb) {
    _engine.updateBounds(_violationId, 0, 0, widenOnly);
    return;
  }

  for (const Int val : std::array<Int, 3>{xUb, yLb, yUb}) {
    if (xLb != val) {
      _engine.updateBounds(_violationId, 0, 1, widenOnly);
      return;
    }
  }
  _engine.updateBounds(_violationId, 1, 1, widenOnly);
}

void NotEqual::recompute(Timestamp ts) {
  updateValue(ts, _violationId, _engine.value(ts, _x) == _engine.value(ts, _y));
}

void NotEqual::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId NotEqual::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void NotEqual::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
