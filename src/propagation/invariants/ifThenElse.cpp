#include "atlantis/propagation/invariants/ifThenElse.hpp"

namespace atlantis::propagation {

static inline size_t toIndex(Int violation) { return violation == 0 ? 0 : 1; }

IfThenElse::IfThenElse(SolverBase& solver, VarId output, VarViewId condition,
                       VarViewId thenBranch, VarViewId elseBranch)
    : Invariant(solver),
      _output(output),
      _condition(condition),
      _branches({thenBranch, elseBranch}) {}

IfThenElse::IfThenElse(SolverBase& solver, VarViewId output,
                       VarViewId condition, VarViewId thenBranch,
                       VarViewId elseBranch)
    : IfThenElse(solver, VarId(output), condition, thenBranch, elseBranch) {
  assert(output.isVar());
}

void IfThenElse::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _branches[1], true);
  _solver.registerInvariantInput(_id, _condition, false);
  _solver.registerInvariantInput(_id, _branches[0], true);
  registerDefinedVar(_output);
}

VarViewId IfThenElse::dynamicInputVar(Timestamp ts) const noexcept {
  return _branches[toIndex(_solver.value(ts, _condition))];
}

void IfThenElse::updateBounds(bool widenOnly) {
  _solver.updateBounds(_output,
                       std::min(_solver.lowerBound(_branches[0]),
                                _solver.lowerBound(_branches[1])),
                       std::max(_solver.upperBound(_branches[0]),
                                _solver.upperBound(_branches[1])),
                       widenOnly);
}

void IfThenElse::recompute(Timestamp ts) {
  const size_t index = toIndex(_solver.value(ts, _condition));

  makeAllDynamicInputsInactive(ts);
  makeDynamicInputActive(ts, LocalId{index});

  updateValue(ts, _output, _solver.value(ts, _branches[index]));
}

void IfThenElse::notifyInputChanged(Timestamp ts, LocalId localId) {
  const size_t index = toIndex(_solver.value(ts, _condition));
  if (localId == size_t{2}) {
    if (_activeIndex != index) {
      assert(_activeIndex < 2);
      makeDynamicInputInactive(ts, _activeIndex);
      makeDynamicInputActive(ts, index);
    }
  }
  updateValue(ts, _output, _solver.value(ts, _branches[index]));
}

VarViewId IfThenElse::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _condition;
    case 1:
      return _branches[toIndex(_solver.value(ts, _condition))];
    default:
      return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentInputChanged(Timestamp ts) {
  updateValue(
      ts, _output,
      _solver.value(ts, _branches[toIndex(_solver.value(ts, _condition))]));
}

void IfThenElse::commit(Timestamp ts) {
  Invariant::commit(ts);
  _activeIndex = toIndex(_solver.committedValue(_condition));
}

}  // namespace atlantis::propagation
