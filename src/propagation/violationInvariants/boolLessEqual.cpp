#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"

namespace atlantis::propagation {

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
BoolLessEqual::BoolLessEqual(SolverBase& solver, VarId violationId, VarViewId x,
                             VarViewId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

BoolLessEqual::BoolLessEqual(SolverBase& solver, VarViewId violationId,
                             VarViewId x, VarViewId y)
    : BoolLessEqual(solver, VarId(violationId), x, y) {
  assert(violationId.isVar());
}

void BoolLessEqual::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, false);
  _solver.registerInvariantInput(_id, _y, false);
  registerDefinedVar(_violationId);
}

void BoolLessEqual::updateBounds(bool widenOnly) {
  _solver.updateBounds(_violationId, 0, 1, widenOnly);
}

void BoolLessEqual::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              static_cast<Int>((_solver.value(ts, _x) == 0) &&
                               (_solver.value(ts, _y) != 0)));
}

void BoolLessEqual::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId BoolLessEqual::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolLessEqual::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
