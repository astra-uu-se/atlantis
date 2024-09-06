#include "atlantis/propagation/invariants/binaryMin.hpp"

#include <cmath>

namespace atlantis::propagation {

BinaryMin::BinaryMin(SolverBase& solver, VarId output, VarViewId x, VarViewId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {}

BinaryMin::BinaryMin(SolverBase& solver, VarViewId output, VarViewId x,
                     VarViewId y)
    : BinaryMin(solver, VarId(output), x, y) {
  assert(output.isVar());
}

void BinaryMin::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, 0, false);
  _solver.registerInvariantInput(_id, _y, 0, false);
  registerDefinedVar(_output);
}

void BinaryMin::updateBounds(bool widenOnly) {
  _solver.updateBounds(
      _output, std::min(_solver.lowerBound(_x), _solver.lowerBound(_y)),
      std::min(_solver.upperBound(_x), _solver.upperBound(_y)), widenOnly);
}

void BinaryMin::recompute(Timestamp ts) {
  updateValue(ts, _output,
              std::min(_solver.value(ts, _x), _solver.value(ts, _y)));
}

VarViewId BinaryMin::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BinaryMin::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void BinaryMin::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation
