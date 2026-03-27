#include "atlantis/propagation/violationInvariants/equal.hpp"

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::propagation {

static Int compute(const Int x, const Int y) {
  Int difference;
  if (sub_overflow(std::max(x, y), std::min(x, y), difference)) {
    return std::numeric_limits<Int>::max();
  }
  assert(difference >= 0);
  return difference;
}

/**
 * Constraint x = y
 * @param solver the solver that the invariant is added to
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
Equal::Equal(SolverBase& solver, const VarId violationId, const VarViewId x, const VarViewId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

Equal::Equal(SolverBase& solver, const VarViewId violationId, const VarViewId x,
             const VarViewId y)
    : Equal(solver, VarId{violationId}, x, y) {
  assert(violationId.isVar());
}

void Equal::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, LocalId{0}, false);
  _solver.registerInvariantInput(_id, _y, LocalId{1}, false);
  registerDefinedVar(_violationId);
}

void Equal::updateBounds(bool widenOnly) {
  const Int xLb = _solver.lowerBound(_x);
  const Int xUb = _solver.upperBound(_x);
  const Int yLb = _solver.lowerBound(_y);
  const Int yUb = _solver.upperBound(_y);

  const Int lb = xLb <= yUb && yLb <= xUb
                     ? 0
                     : std::min(overflow::saturatingAbsDiff(xLb, yUb),
                                overflow::saturatingAbsDiff(yLb, xUb));

  const Int ub = std::max(std::max(overflow::saturatingAbsDiff(xLb, yLb),
                                   overflow::saturatingAbsDiff(xLb, yUb)),
                          std::max(overflow::saturatingAbsDiff(xUb, yLb),
                                   overflow::saturatingAbsDiff(xUb, yUb)));

  _solver.updateBounds(_violationId, lb, ub, widenOnly);
}

void Equal::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              overflow::saturatingAbsDiff(_solver.value(ts, _x),
                                          _solver.value(ts, _y)));
}

void Equal::notifyInputChanged(const Timestamp ts, LocalId) { recompute(ts); }

VarViewId Equal::nextInput(const Timestamp ts) {
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
