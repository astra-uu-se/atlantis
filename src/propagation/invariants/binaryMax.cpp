#include "atlantis/propagation/invariants/binaryMax.hpp"

namespace atlantis::propagation {

BinaryMax::BinaryMax(SolverBase& solver, VarId output, VarId x, VarId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BinaryMax::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, 0, false);
  _solver.registerInvariantInput(_id, _y, 0, false);
  registerDefinedVar(_output);
}

void BinaryMax::updateBounds(bool widenOnly) {
  _solver.updateBounds(
      _output, std::max(_solver.lowerBound(_x), _solver.lowerBound(_y)),
      std::max(_solver.upperBound(_x), _solver.upperBound(_y)), widenOnly);
}

void BinaryMax::recompute(Timestamp ts) {
  updateValue(ts, _output,
              std::max(_solver.value(ts, _x), _solver.value(ts, _y)));
}

VarId BinaryMax::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BinaryMax::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void BinaryMax::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation
