#include "atlantis/propagation/violationInvariants/lessEqual.hpp"

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::propagation {

static Int compute(const Int x, const Int y) {
  return x <= y ? 0 : overflow::saturatingSub(x, y);
}

/**
 * Constraint x <= y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */

LessEqual::LessEqual(SolverBase& solver, const VarId violationId, const VarViewId x,
                     const VarViewId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

LessEqual::LessEqual(SolverBase& solver, const VarViewId violationId, const VarViewId x,
                     const VarViewId y)
    : LessEqual(solver, VarId{violationId}, x, y) {
  assert(violationId.isVar());
}

void LessEqual::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, LocalId{0}, false);
  _solver.registerInvariantInput(_id, _y, LocalId{1}, false);
  registerDefinedVar(_violationId);
}

void LessEqual::updateBounds(const bool widenOnly) {
  _solver.updateBounds(
      _violationId,
      compute(_solver.lowerBound(_x), _solver.upperBound(_y)),
      compute(_solver.upperBound(_x), _solver.lowerBound(_y)),
      widenOnly);
}

void LessEqual::recompute(const Timestamp ts) {
  updateValue(ts, _violationId,
              compute(_solver.value(ts, _x), _solver.value(ts, _y)));
}

void LessEqual::notifyInputChanged(const Timestamp ts, LocalId) { recompute(ts); }

VarViewId LessEqual::nextInput(const Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void LessEqual::notifyCurrentInputChanged(const Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
