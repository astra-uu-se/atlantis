#include "atlantis/propagation/violationInvariants/powDomain.hpp"

namespace atlantis::propagation {

/**
 * Constraint x != 0 && y >= 0
 * Required for Pow invariants since pow(0, v) where v < 0 is undefined.
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y parameter of rhs
 */
PowDomain::PowDomain(SolverBase& solver, VarId violationId, VarViewId x,
                     VarViewId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

PowDomain::PowDomain(SolverBase& solver, VarViewId violationId, VarViewId x,
                     VarViewId y)
    : PowDomain(solver, VarId(violationId), x, y) {
  assert(violationId.isVar());
}

void PowDomain::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, false);
  _solver.registerInvariantInput(_id, _y, false);
  registerDefinedVar(_violationId);
}

void PowDomain::updateBounds(bool widenOnly) {
  const Int xLb = _solver.lowerBound(_x);
  const Int xUb = _solver.upperBound(_x);
  const Int yLb = _solver.lowerBound(_y);
  const Int yUb = _solver.upperBound(_y);

  const Int lb = xLb == 0 && xUb == 0 && yUb < 0 ? 1 : 0;
  const Int ub = xLb <= 0 && 0 <= xUb && yLb < 0 ? 1 : 0;

  _solver.updateBounds(_violationId, lb, ub, widenOnly);
}

void PowDomain::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              _solver.value(ts, _x) == 0 && _solver.value(ts, _y) < 0 ? 1 : 0);
}

void PowDomain::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId PowDomain::nextInput(Timestamp ts) {
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

[[nodiscard]] bool PowDomain::shouldPost(SolverBase& solver, VarViewId x,
                                         VarViewId y) {
  return solver.lowerBound(x) <= 0 && 0 <= solver.upperBound(x) &&
         solver.lowerBound(y) < 0;
}
}  // namespace atlantis::propagation
