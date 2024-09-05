#include "atlantis/propagation/invariants/forAll.hpp"

#include <limits>
#include <vector>

namespace atlantis::propagation {

ForAll::ForAll(SolverBase& solver, VarId output,
               std::vector<VarViewId>&& varArray)
    : Invariant(solver),
      _output(output),
      _varArray(std::move(varArray)),
      _localPriority(_varArray.size()) {}

ForAll::ForAll(SolverBase& solver, VarViewId output,
               std::vector<VarViewId>&& varArray)
    : ForAll(solver, VarId(output), std::move(varArray)) {
  assert(output.isVar());
}

void ForAll::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  registerDefinedVar(_output);
}

void ForAll::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();
  for (const VarViewId& input : _varArray) {
    lb = std::max(lb, _solver.lowerBound(input));
    ub = std::max(ub, _solver.upperBound(input));
  }
  _solver.updateBounds(_output, std::max(Int(0), lb), ub, widenOnly);
}

void ForAll::recompute(Timestamp ts) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, _solver.value(ts, _varArray[i]));
  }
  assert(_localPriority.minPriority(ts) >= 0);
  updateValue(ts, _output, _localPriority.maxPriority(ts));
}

void ForAll::notifyInputChanged(Timestamp ts, LocalId id) {
  _localPriority.updatePriority(
      ts, id, std::max(Int(0), _solver.value(ts, _varArray[id])));
  assert(_localPriority.minPriority(ts) >= 0);
  updateValue(ts, _output, _localPriority.maxPriority(ts));
}

VarViewId ForAll::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void ForAll::notifyCurrentInputChanged(Timestamp ts) {
  notifyInputChanged(ts, _state.value(ts));
}

void ForAll::commit(Timestamp ts) {
  Invariant::commit(ts);
  _localPriority.commitIf(ts);
}
}  // namespace atlantis::propagation
