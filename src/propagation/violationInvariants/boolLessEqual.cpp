#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

/**
 * Constraint x = y
 * @param solver the solver that the invariant is added to
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
BoolLessEqual::BoolLessEqual(SolverBase& solver, const VarId violationId, const VarViewId x,
                             const VarViewId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

BoolLessEqual::BoolLessEqual(SolverBase& solver, const VarViewId violationId,
                             const VarViewId x, const VarViewId y)
    : BoolLessEqual(solver, VarId{violationId}, x, y) {
  assert(violationId.isVar());
}

void BoolLessEqual::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, LocalId{0}, false);
  _solver.registerInvariantInput(_id, _y, LocalId{1}, false);
  registerDefinedVar(_violationId);
}

void BoolLessEqual::updateBounds(const bool widenOnly) {
  _solver.updateBounds(_violationId, 0, 1, widenOnly);
}

void BoolLessEqual::recompute(const Timestamp ts) {
  updateValue(
      ts, _violationId,
      (_solver.value(ts, _x) != 0) || (_solver.value(ts, _y) == 0) ? 0 : 1);
}

void BoolLessEqual::notifyInputChanged(const Timestamp ts, LocalId) { recompute(ts); }

VarViewId BoolLessEqual::nextInput(const Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolLessEqual::notifyCurrentInputChanged(const Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
