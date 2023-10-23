#include "propagation/constraints/powDomain.hpp"

namespace atlantis::propagation {

/**
 * Constraint x != 0 && y >= 0
 * Required for Pow invariants since pow(0, v) where v < 0 is undefined.
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y parameter of rhs
 */
PowDomain::PowDomain(Engine& engine, VarId violationId, VarId x, VarId y)
    : Constraint(engine, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void PowDomain::registerVars() {
  assert(_id != NULL_ID);
  _engine.registerInvariantInput(_id, _x, LocalId(0));
  _engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(_violationId);
}

void PowDomain::updateBounds(bool widenOnly) {
  const Int xLb = _engine.lowerBound(_x);
  const Int xUb = _engine.upperBound(_x);
  const Int yLb = _engine.lowerBound(_y);
  const Int yUb = _engine.upperBound(_y);

  const Int lb = xLb == 0 && xUb == 0 && yUb < 0 ? 1 : 0;
  const Int ub = xLb <= 0 && 0 <= xUb && yLb < 0 ? 1 : 0;

  _engine.updateBounds(_violationId, lb, ub, widenOnly);
}

void PowDomain::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              _engine.value(ts, _x) == 0 && _engine.value(ts, _y) < 0 ? 1 : 0);
}

void PowDomain::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId PowDomain::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void PowDomain::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

[[nodiscard]] bool PowDomain::shouldPost(Engine& engine, VarId x, VarId y) {
  return engine.lowerBound(x) <= 0 && 0 <= engine.upperBound(x) &&
         engine.lowerBound(y) < 0;
}
}  // namespace atlantis::propagation