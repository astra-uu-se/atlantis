#include "atlantis/propagation/invariants/plus.hpp"

#include <cmath>

namespace atlantis::propagation {

Plus::Plus(SolverBase& solver, VarId output, VarId x, VarId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Plus::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, 0, false);
  _solver.registerInvariantInput(_id, _y, 0, false);
  registerDefinedVar(_output);
}

void Plus::updateBounds(bool widenOnly) {
  _solver.updateBounds(_output, _solver.lowerBound(_x) + _solver.lowerBound(_y),
                       _solver.upperBound(_x) + _solver.upperBound(_y),
                       widenOnly);
}

void Plus::recompute(Timestamp ts) {
  updateValue(ts, _output, _solver.value(ts, _x) + _solver.value(ts, _y));
}

VarId Plus::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Plus::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void Plus::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

}  // namespace atlantis::propagation
