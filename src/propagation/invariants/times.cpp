#include "atlantis/propagation/invariants/times.hpp"

#include <algorithm>
#include <array>

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

Times::Times(SolverBase& solver, const VarId output, const VarViewId x,
             const VarViewId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {}

Times::Times(SolverBase& solver, const VarViewId output, const VarViewId x,
             const VarViewId y)
    : Times(solver, VarId{output}, x, y) {
  assert(output.isVar());
}

void Times::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, 0, false);
  _solver.registerInvariantInput(_id, _y, 0, false);
  registerDefinedVar(_output);
}

void Times::updateBounds(const bool widenOnly) {
  const Int xLb = _solver.lowerBound(_x);
  const Int xUb = _solver.upperBound(_x);
  const Int yLb = _solver.lowerBound(_y);
  const Int yUb = _solver.upperBound(_y);
  const std::array<const Int, 4> vals{xLb * yLb, xLb * yUb, xUb * yLb,
                                      xUb * yUb};
  const auto [lb, ub] = std::minmax_element(vals.begin(), vals.end());
  _solver.updateBounds(_output, *lb, *ub, widenOnly);
}

void Times::recompute(const Timestamp ts) {
  updateValue(ts, _output, _solver.value(ts, _x) * _solver.value(ts, _y));
}

VarViewId Times::nextInput(const Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return VAR_VIEW_NULL_ID;
  }
}

void Times::notifyCurrentInputChanged(const Timestamp ts) { recompute(ts); }

void Times::notifyInputChanged(const Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation
