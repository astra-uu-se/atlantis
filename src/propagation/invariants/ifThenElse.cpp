#include "propagation/invariants/ifThenElse.hpp"

namespace atlantis::propagation {

IfThenElse::IfThenElse(SolverBase& solver, VarId output, VarId b, VarId x, VarId y)
    : Invariant(solver), _output(output), _b(b), _xy({x, y}) {
  _modifiedVars.reserve(1);
}

void IfThenElse::registerVars() {
  assert(!_id.equals(NULL_ID));
  _solver.registerInvariantInput(_id, _b, 0);
  _solver.registerInvariantInput(_id, _xy[0], 0, true);
  _solver.registerInvariantInput(_id, _xy[1], 0, true);
  registerDefinedVariable(_output);
}

VarId IfThenElse::dynamicInputVar(Timestamp ts) const noexcept {
  return _solver.value(ts,
                       _xy[static_cast<size_t>(_solver.value(ts, _b) != 0)]);
}

void IfThenElse::updateBounds(bool widenOnly) {
  _solver.updateBounds(
      _output, std::min(_solver.lowerBound(_xy[0]), _solver.lowerBound(_xy[1])),
      std::max(_solver.upperBound(_xy[0]), _solver.upperBound(_xy[1])),
      widenOnly);
}

void IfThenElse::recompute(Timestamp ts) {
  updateValue(
      ts, _output,
      _solver.value(ts, _xy[static_cast<size_t>(_solver.value(ts, _b) != 0)]));
}

void IfThenElse::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId IfThenElse::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _b;
    case 1:
      return _xy[1 - (_solver.value(ts, _b) == 0)];
    default:
      return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation