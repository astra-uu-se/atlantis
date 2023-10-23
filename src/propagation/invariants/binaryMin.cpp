#include "propagation/invariants/binaryMin.hpp"

namespace atlantis::propagation {

BinaryMin::BinaryMin(SolverBase& solver, VarId output, VarId x, VarId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BinaryMin::registerVars() {
  assert(!_id.equals(NULL_ID));
  _solver.registerInvariantInput(_id, _x, 0);
  _solver.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(_output);
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

VarId BinaryMin::nextInput(Timestamp ts) {
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