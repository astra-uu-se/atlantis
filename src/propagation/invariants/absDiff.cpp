#include "atlantis/propagation/invariants/absDiff.hpp"

#include <algorithm>

namespace atlantis::propagation {

AbsDiff::AbsDiff(SolverBase& solver, VarId output, VarViewId x, VarViewId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {}

AbsDiff::AbsDiff(SolverBase& solver, VarViewId output, VarViewId x, VarViewId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {
  assert(output.isVar());
}

void AbsDiff::registerVars() {
  assert(_id != NULL_ID);

  registerDefinedVar(_output);
  _solver.registerInvariantInput(_id, _x, false);
  _solver.registerInvariantInput(_id, _y, false);
}

void AbsDiff::updateBounds(bool widenOnly) {
  const Int xLb = _solver.lowerBound(_x);
  const Int xUb = _solver.upperBound(_x);
  const Int yLb = _solver.lowerBound(_y);
  const Int yUb = _solver.upperBound(_y);

  const Int lb = xLb <= yUb && yLb <= xUb
                     ? 0
                     : std::min(std::abs(xLb - yUb), std::abs(yLb - xUb));

  const Int ub = std::max(std::max(std::abs(xLb - yLb), std::abs(xLb - yUb)),
                          std::max(std::abs(xUb - yLb), std::abs(xUb - yUb)));

  _solver.updateBounds(_output, lb, ub, widenOnly);
}

void AbsDiff::recompute(Timestamp ts) {
  updateValue(ts, _output,
              std::abs(_solver.value(ts, _x) - _solver.value(ts, _y)));
}

void AbsDiff::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId AbsDiff::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void AbsDiff::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
