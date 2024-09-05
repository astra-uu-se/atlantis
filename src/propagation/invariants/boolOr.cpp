#include "atlantis/propagation/invariants/boolOr.hpp"

#include "atlantis/propagation/variables/intVar.hpp"

namespace atlantis::propagation {

/**
 * Invariant output = x \/ y. output does not violate if x or y does not
 * violate.
 * @param violationId id for the violationCount
 * @param x first violation variable
 * @param y second violation variable
 * @param output result
 */
BoolOr::BoolOr(SolverBase& solver, VarId output, VarViewId x, VarViewId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {}

BoolOr::BoolOr(SolverBase& solver, VarViewId output, VarViewId x, VarViewId y)
    : BoolOr(solver, VarId(output), x, y) {
  assert(output.isVar());
}

void BoolOr::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, LocalId(0), false);
  _solver.registerInvariantInput(_id, _y, LocalId(0), false);
  registerDefinedVar(_output);
}

void BoolOr::updateBounds(bool widenOnly) {
  _solver.updateBounds(
      _output, std::min(_solver.lowerBound(_x), _solver.lowerBound(_y)),
      std::min(_solver.upperBound(_x), _solver.upperBound(_y)), widenOnly);
}

void BoolOr::recompute(Timestamp ts) {
  updateValue(ts, _output,
              std::min(_solver.value(ts, _x), _solver.value(ts, _y)));
}

void BoolOr::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId BoolOr::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolOr::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
