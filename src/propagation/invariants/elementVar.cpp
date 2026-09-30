#include "atlantis/propagation/invariants/elementVar.hpp"

#include <algorithm>
#include <limits>
#include <vector>

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

inline size_t ElementVar::safeIndex(const Int index) const noexcept {
  return std::max<Int>(
      0, std::min(static_cast<Int>(_varArray.size()) - 1, index - _offset));
}
ElementVar::ElementVar(SolverBase& solver, const VarId output,
                       const VarViewId index, std::vector<VarViewId>&& varArray,
                       const Int offset)
    : Invariant(solver),
      _output(output),
      _index(index),
      _varArray(std::move(varArray)),
      _offset(offset) {}

ElementVar::ElementVar(SolverBase& solver, const VarViewId output,
                       const VarViewId index, std::vector<VarViewId>&& varArray,
                       const Int offset)
    : ElementVar(solver, VarId{output}, index, std::move(varArray), offset) {
  assert(output.isVar());
}

void ElementVar::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _index, 0, false);
  for (const VarViewId& input : _varArray) {
    _solver.registerInvariantInput(_id, input, 0, true);
  }
  registerDefinedVar(_output);
}

void ElementVar::updateBounds(const bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();
  Int iLb = std::max<Int>(_offset, _solver.lowerBound(_index));
  Int iUb = std::min<Int>(static_cast<Int>(_varArray.size()) - 1 + _offset,
                          _solver.upperBound(_index));
  if (iLb > iUb) {
    iLb = _offset;
    iUb = static_cast<Int>(_varArray.size()) - 1 + _offset;
  }
  for (Int i = iLb; i <= iUb; ++i) {
    assert(_offset <= i);
    assert(i - _offset < static_cast<Int>(_varArray.size()));
    lb = std::min(lb, _solver.lowerBound(_varArray[safeIndex(i)]));
    ub = std::max(ub, _solver.upperBound(_varArray[safeIndex(i)]));
  }
  _solver.updateBounds(_output, lb, ub, widenOnly);
}

void ElementVar::recompute(const Timestamp ts) {
  assert(safeIndex(_solver.value(ts, _index)) < _varArray.size());
  updateValue(
      ts, _output,
      _solver.value(ts, _varArray[safeIndex(_solver.value(ts, _index))]));
}

VarViewId ElementVar::dynamicInputVar(const Timestamp ts) const noexcept {
  return _varArray[safeIndex(_solver.value(ts, _index))];
}

void ElementVar::notifyInputChanged(const Timestamp ts, LocalId) {
  recompute(ts);
}

VarViewId ElementVar::nextInput(const Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _index;
    case 1: {
      assert(safeIndex(_solver.value(ts, _index)) < _varArray.size());
      return _varArray[safeIndex(_solver.value(ts, _index))];
    }
    default:
      return VAR_VIEW_NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentInputChanged(const Timestamp ts) {
  recompute(ts);
}
}  // namespace atlantis::propagation
