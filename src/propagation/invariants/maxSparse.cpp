#include "atlantis/propagation/invariants/maxSparse.hpp"

#include <limits>

namespace atlantis::propagation {

MaxSparse::MaxSparse(SolverBase& solver, VarId output,
                     std::vector<VarViewId>&& varArray)
    : Invariant(solver),
      _output(output),
      _varArray(std::move(varArray)),
      _localPriority(_varArray.size()) {
  assert(!_varArray.empty());
}

MaxSparse::MaxSparse(SolverBase& solver, VarViewId output,
                     std::vector<VarViewId>&& varArray)
    : MaxSparse(solver, VarId(output), std::move(varArray)) {
  assert(output.isVar());
}

void MaxSparse::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  registerDefinedVar(_output);
}

void MaxSparse::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();
  for (const VarViewId& input : _varArray) {
    lb = std::max(lb, _solver.lowerBound(input));
    ub = std::max(ub, _solver.upperBound(input));
  }
  _solver.updateBounds(_output, lb, ub, widenOnly);
}

void MaxSparse::recompute(Timestamp ts) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, _solver.value(ts, _varArray[i]));
  }
  updateValue(ts, _output, _localPriority.maxPriority(ts));
}

void MaxSparse::notifyInputChanged(Timestamp ts, LocalId id) {
  _localPriority.updatePriority(ts, id, _solver.value(ts, _varArray[id]));
  updateValue(ts, _output, _localPriority.maxPriority(ts));
}

VarViewId MaxSparse::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void MaxSparse::notifyCurrentInputChanged(Timestamp ts) {
  notifyInputChanged(ts, _state.value(ts));
}

void MaxSparse::commit(Timestamp ts) {
  Invariant::commit(ts);
  _localPriority.commitIf(ts);
}
}  // namespace atlantis::propagation
