#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"

#include <limits>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::propagation {

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
BoolLessThan::BoolLessThan(SolverBase& solver, const VarId violationId, const VarViewId x,
                           const VarViewId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

BoolLessThan::BoolLessThan(SolverBase& solver, const VarViewId violationId,
                           const VarViewId x, const VarViewId y)
    : BoolLessThan(solver, VarId{violationId}, x, y) {
  assert(violationId.isVar());
}

void BoolLessThan::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, LocalId{0}, false);
  _solver.registerInvariantInput(_id, _y, LocalId{1}, false);
  registerDefinedVar(_violationId);
}

void BoolLessThan::updateBounds(const bool widenOnly) {
  _solver.updateBounds(_violationId, 0, 1, widenOnly);
}

void BoolLessThan::recompute(const Timestamp ts) {
  updateValue(ts, _violationId, overflow::saturatingAdd(_solver.value(ts, _x) == 0 ? 1 : 0, _solver.value(ts, _y)));
}

void BoolLessThan::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId BoolLessThan::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolLessThan::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
