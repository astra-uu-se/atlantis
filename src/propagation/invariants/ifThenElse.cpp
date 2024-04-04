#include "atlantis/propagation/invariants/ifThenElse.hpp"

namespace atlantis::propagation {

static inline size_t toIndex(Int violation) {
  return static_cast<size_t>(violation != 0);
}

IfThenElse::IfThenElse(SolverBase& solver, VarId output, VarId b, VarId x,
                       VarId y)
    : Invariant(solver), _output(output), _b(b), _xy({x, y}) {}

void IfThenElse::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _xy[0], true);
  _solver.registerInvariantInput(_id, _xy[1], true);
  _solver.registerInvariantInput(_id, _b, false);
  registerDefinedVar(_output);
}

VarId IfThenElse::dynamicInputVar(Timestamp ts) const noexcept {
  return _solver.value(ts, _xy[toIndex(_solver.value(ts, _b))]);
}

void IfThenElse::updateBounds(bool widenOnly) {
  _solver.updateBounds(
      _output, std::min(_solver.lowerBound(_xy[0]), _solver.lowerBound(_xy[1])),
      std::max(_solver.upperBound(_xy[0]), _solver.upperBound(_xy[1])),
      widenOnly);
}

void IfThenElse::recompute(Timestamp ts) {
  const size_t index = toIndex(_solver.value(ts, _b));
  makeDynamicInputInactive(ts, LocalId(toIndex(_solver.value(ts, _b) == 0)));
  makeDynamicInputActive(ts, LocalId(index));

  updateValue(ts, _output, _solver.value(ts, _xy[index]));
}

void IfThenElse::notifyInputChanged(Timestamp ts, LocalId localId) {
  const size_t index = toIndex(_solver.value(ts, _b));
  if (localId == size_t(2)) {
    makeDynamicInputInactive(ts, LocalId(_committedViolation));
    makeDynamicInputActive(ts, LocalId(index));
  }
  updateValue(ts, _output, _solver.value(ts, _xy[index]));
}

VarId IfThenElse::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _b;
    case 1:
      return _xy[toIndex(_solver.value(ts, _b))];
    default:
      return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentInputChanged(Timestamp ts) {
  updateValue(ts, _output,
              _solver.value(ts, _xy[toIndex(_solver.value(ts, _b))]));
}

void IfThenElse::commit(Timestamp ts) {
  Invariant::commit(ts);
  _committedViolation = toIndex(_solver.committedValue(_b));
}

}  // namespace atlantis::propagation
