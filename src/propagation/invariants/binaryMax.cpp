#include "atlantis/propagation/invariants/binaryMax.hpp"

#include <cmath>

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

BinaryMax::BinaryMax(SolverBase& solver, const VarId output, const VarViewId x,
                     const VarViewId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {}

BinaryMax::BinaryMax(SolverBase& solver, const VarViewId output, const VarViewId x,
                     const VarViewId y)
    : BinaryMax(solver, VarId{output}, x, y) {
  assert(output.isVar());
}

void BinaryMax::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, 0, false);
  _solver.registerInvariantInput(_id, _y, 0, false);
  registerDefinedVar(_output);
}

void BinaryMax::updateBounds(const bool widenOnly) {
  _solver.updateBounds(
      _output, std::max(_solver.lowerBound(_x), _solver.lowerBound(_y)),
      std::max(_solver.upperBound(_x), _solver.upperBound(_y)), widenOnly);
}

void BinaryMax::recompute(const Timestamp ts) {
  updateValue(ts, _output,
              std::max(_solver.value(ts, _x), _solver.value(ts, _y)));
}

VarViewId BinaryMax::nextInput(const Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return VAR_VIEW_NULL_ID;
  }
}

void BinaryMax::notifyCurrentInputChanged(const Timestamp ts) { recompute(ts); }

void BinaryMax::notifyInputChanged(const Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation
