#include "atlantis/propagation/invariants/ifThenElse.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

IfThenElse::IfThenElse(SolverBase& solver, const VarId output,
                       const VarViewId condition, const VarViewId thenVar,
                       const VarViewId elseVar)
    : Invariant(solver),
      _output(output),
      _condition(condition),
      _branches({thenVar, elseVar}) {}

IfThenElse::IfThenElse(SolverBase& solver, const VarViewId output,
                       const VarViewId condition, const VarViewId thenVar,
                       const VarViewId elseVar)
    : IfThenElse(solver, VarId{output}, condition, thenVar, elseVar) {
  assert(output.isVar());
}

void IfThenElse::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _condition, 0, false);
  _solver.registerInvariantInput(_id, _branches[0], 0, true);
  _solver.registerInvariantInput(_id, _branches[1], 0, true);
  registerDefinedVar(_output);
}

VarViewId IfThenElse::dynamicInputVar(const Timestamp ts) const noexcept {
  return _branches[_solver.value(ts, _condition) != 0];
}

void IfThenElse::updateBounds(const bool widenOnly) {
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

void IfThenElse::recompute(const Timestamp ts) {
  updateValue(
      ts, _output,
      _solver.value(
          ts,
          _branches[static_cast<size_t>(_solver.value(ts, _condition) != 0)]));
}

void IfThenElse::notifyInputChanged(const Timestamp ts, LocalId) {
  recompute(ts);
}

VarViewId IfThenElse::nextInput(const Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _condition;
    case 1:
      return _branches[1 - (_solver.value(ts, _condition) == 0)];
    default:
      return VAR_VIEW_NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentInputChanged(const Timestamp ts) {
  recompute(ts);
}
}  // namespace atlantis::propagation
