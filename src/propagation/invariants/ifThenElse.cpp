#include "atlantis/propagation/invariants/ifThenElse.hpp"

namespace atlantis::propagation {

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
  _solver.registerInvariantInput(_id, _condition, 0, false);
  _solver.registerInvariantInput(_id, _branches[0], 0, true);
  _solver.registerInvariantInput(_id, _branches[1], 0, true);
  registerDefinedVar(_output);
}

VarViewId IfThenElse::dynamicInputVar(Timestamp ts) const noexcept {
  return _solver.value(
      ts, _branches[static_cast<size_t>(_solver.value(ts, _condition) != 0)]);
}

void IfThenElse::updateBounds(bool widenOnly) {
  if (_solver.lowerBound(_condition) == 0 &&
      _solver.upperBound(_condition) == 0) {
    _solver.updateBounds(_output, _solver.lowerBound(_branches[0]),
                         _solver.upperBound(_branches[0]), widenOnly);
  } else if (_solver.lowerBound(_condition) > 0) {
    _solver.updateBounds(_output, _solver.lowerBound(_branches[1]),
                         _solver.upperBound(_branches[1]), widenOnly);
  } else {
    _solver.updateBounds(_output,
                         std::min(_solver.lowerBound(_branches[0]),
                                  _solver.lowerBound(_branches[1])),
                         std::max(_solver.upperBound(_branches[0]),
                                  _solver.upperBound(_branches[1])),
                         widenOnly);
  }
}

void IfThenElse::recompute(Timestamp ts) {
  updateValue(
      ts, _output,
      _solver.value(
          ts,
          _branches[static_cast<size_t>(_solver.value(ts, _condition) != 0)]));
}

void IfThenElse::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId IfThenElse::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _condition;
    case 1:
      return _branches[1 - (_solver.value(ts, _condition) == 0)];
    default:
      return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
