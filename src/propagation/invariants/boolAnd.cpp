#include "atlantis/propagation/invariants/boolAnd.hpp"

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/variables/intVar.hpp"

namespace atlantis::propagation {

/**
 * Invariant output = x /\ y
 * output does not violate if x and y does not violate
 * @param solver the solver that the invariant is added to
 * @param output id for the output
 * @param x first violation variable
 * @param y second violation variable
 * @param output the result
 */
BoolAnd::BoolAnd(SolverBase& solver, const VarId output, const VarViewId x,
                 const VarViewId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {}

BoolAnd::BoolAnd(SolverBase& solver, const VarViewId output, const VarViewId x,
                 const VarViewId y)
    : BoolAnd(solver, VarId{output}, x, y) {
  assert(output.isVar());
}

void BoolAnd::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, 0, false);
  _solver.registerInvariantInput(_id, _y, 0, false);
  registerDefinedVar(_output);
}

void BoolAnd::updateBounds(const bool widenOnly) {
  _solver.updateBounds(
      _output, std::max(_solver.lowerBound(_x), _solver.lowerBound(_y)),
      std::max(_solver.upperBound(_x), _solver.upperBound(_y)), widenOnly);
}

void BoolAnd::recompute(const Timestamp ts) {
  updateValue(ts, _output,
              std::max(_solver.value(ts, _x), _solver.value(ts, _y)));
}

void BoolAnd::notifyInputChanged(const Timestamp ts, LocalId) { recompute(ts); }

VarViewId BoolAnd::nextInput(const Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return VAR_VIEW_NULL_ID;
  }
}

void BoolAnd::notifyCurrentInputChanged(const Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
