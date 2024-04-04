#include "atlantis/propagation/violationInvariants/equal.hpp"

namespace atlantis::propagation {

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
Equal::Equal(SolverBase& solver, VarId violationId, VarViewId x, VarViewId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

Equal::Equal(SolverBase& solver, VarViewId violationId, VarViewId x,
             VarViewId y)
    : Equal(solver, VarId(violationId), x, y) {
  assert(violationId.isVar());
}

void Equal::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, false);
  _solver.registerInvariantInput(_id, _y, false);
  registerDefinedVar(_violationId);
}

void Equal::updateBounds(bool widenOnly) {
  const Int xLb = _solver.lowerBound(_x);
  const Int xUb = _solver.upperBound(_x);
  const Int yLb = _solver.lowerBound(_y);
  const Int yUb = _solver.upperBound(_y);

  const Int lb = xLb <= yUb && yLb <= xUb
                     ? 0
                     : std::min(std::abs(xLb - yUb), std::abs(yLb - xUb));

  const Int ub = std::max(std::max(std::abs(xLb - yLb), std::abs(xLb - yUb)),
                          std::max(std::abs(xUb - yLb), std::abs(xUb - yUb)));

  _solver.updateBounds(_violationId, lb, ub, widenOnly);
}

void Equal::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              std::abs(_solver.value(ts, _x) - _solver.value(ts, _y)));
}

void Equal::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId Equal::nextInput(Timestamp ts) {
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
}  // namespace atlantis::propagation
