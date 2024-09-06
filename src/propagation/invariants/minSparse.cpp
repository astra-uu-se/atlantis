#include "atlantis/propagation/invariants/minSparse.hpp"

#include <limits>
#include <utility>

namespace atlantis::propagation {

MinSparse::MinSparse(SolverBase& solver, VarId output,
                     std::vector<VarViewId>&& varArray)
    : Invariant(solver),
      _output(output),
      _varArray(std::move(varArray)),
      _localPriority(_varArray.size()) {
  assert(!_varArray.empty());
}

MinSparse::MinSparse(SolverBase& solver, VarViewId output,
                     std::vector<VarViewId>&& varArray)
    : MinSparse(solver, VarId(output), std::move(varArray)) {
  assert(output.isVar());
}

void MinSparse::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  registerDefinedVar(_output);
}

void MinSparse::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::max();
  for (const VarViewId& input : _varArray) {
    lb = std::min(lb, _solver.lowerBound(input));
    ub = std::min(ub, _solver.upperBound(input));
  }
  _solver.updateBounds(_output, lb, ub, widenOnly);
}

void MinSparse::recompute(Timestamp ts) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, _solver.value(ts, _varArray[i]));
  }
  updateValue(ts, _output, _localPriority.minPriority(ts));
}

void MinSparse::notifyInputChanged(Timestamp ts, LocalId id) {
  _localPriority.updatePriority(ts, id, _solver.value(ts, _varArray[id]));
  updateValue(ts, _output, _localPriority.minPriority(ts));
}

VarViewId MinSparse::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void MinSparse::notifyCurrentInputChanged(Timestamp ts) {
  notifyInputChanged(ts, _state.value(ts));
}

void MinSparse::commit(Timestamp ts) {
  Invariant::commit(ts);
  _localPriority.commitIf(ts);
}
}  // namespace atlantis::propagation
