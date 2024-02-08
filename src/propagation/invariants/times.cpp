#include "propagation/invariants/times.hpp"

namespace atlantis::propagation {

Times::Times(SolverBase& solver, VarId output, VarId x, VarId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Times::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, 0, false);
  _solver.registerInvariantInput(_id, _y, 0, false);
  registerDefinedVar(_output);
}

void Times::updateBounds(bool widenOnly) {
  const Int xLb = _solver.lowerBound(_x);
  const Int xUb = _solver.upperBound(_x);
  const Int yLb = _solver.lowerBound(_y);
  const Int yUb = _solver.upperBound(_y);
  const std::array<const Int, 4> vals{xLb * yLb, xLb * yUb, xUb * yLb,
                                      xUb * yUb};
  const auto [lb, ub] = std::minmax_element(vals.begin(), vals.end());
  _solver.updateBounds(_output, *lb, *ub, widenOnly);
}

void Times::recompute(Timestamp ts) {
  updateValue(ts, _output, _solver.value(ts, _x) * _solver.value(ts, _y));
}

VarId Times::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Times::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void Times::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation