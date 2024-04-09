#include "atlantis/propagation/violationInvariants/lessEqual.hpp"

namespace atlantis::propagation {

/**
 * Constraint x <= y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
LessEqual::LessEqual(SolverBase& solver, VarId violationId, VarId x, VarId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

void LessEqual::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, LocalId(0), false);
  _solver.registerInvariantInput(_id, _y, LocalId(0), false);
  registerDefinedVar(_violationId);
}

void LessEqual::updateBounds(bool widenOnly) {
  _solver.updateBounds(
      _violationId,
      std::max(Int(0), _solver.lowerBound(_x) - _solver.upperBound(_y)),
      std::max(Int(0), _solver.upperBound(_x) - _solver.lowerBound(_y)),
      widenOnly);
}

void LessEqual::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              std::max(Int(0), _solver.value(ts, _x) - _solver.value(ts, _y)));
}

void LessEqual::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId LessEqual::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void LessEqual::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
