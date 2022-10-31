#include "constraints/equal.hpp"

#include "core/engine.hpp"

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
Equal::Equal(Engine& engine, VarId violationId, VarId x, VarId y)
    : Constraint(engine, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Equal::registerVars() {
  assert(_id != NULL_ID);
  _engine.registerInvariantInput(_id, _x, LocalId(0));
  _engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(_violationId);
}

void Equal::updateBounds(bool widenOnly) {
  const Int xLb = _engine.lowerBound(_x);
  const Int xUb = _engine.upperBound(_x);
  const Int yLb = _engine.lowerBound(_y);
  const Int yUb = _engine.upperBound(_y);

  const Int lb = xLb <= yUb && yLb <= xUb
                     ? 0
                     : std::min(std::abs(xLb - yUb), std::abs(yLb - xUb));

  const Int ub = std::max(std::max(std::abs(xLb - yLb), std::abs(xLb - yUb)),
                          std::max(std::abs(xUb - yLb), std::abs(xUb - yUb)));

  _engine.updateBounds(_violationId, lb, ub, widenOnly);
}

void Equal::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              std::abs(_engine.value(ts, _x) - _engine.value(ts, _y)));
}

void Equal::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId Equal::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Equal::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void Equal::commit(Timestamp ts) { Invariant::commit(ts); }
