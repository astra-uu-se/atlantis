#include "atlantis/propagation/invariants/elementVar.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace atlantis::propagation {

ElementVar::ElementVar(SolverBase& solver, VarId output, VarViewId index,
                       std::vector<VarViewId>&& varArray, Int offset)
    : Invariant(solver),
      _output(output),
      _index(index),
      _varArray(std::move(varArray)),
      _offset(offset),
      _activeIndex(_varArray.size()) {}

ElementVar::ElementVar(SolverBase& solver, VarViewId output, VarViewId index,
                       std::vector<VarViewId>&& varArray, Int offset)
    : ElementVar(solver, VarId(output), index, std::move(varArray), offset) {
  assert(output.isVar());
}

void ElementVar::registerVars() {
  assert(_id != NULL_ID);
  for (const VarViewId& input : _varArray) {
    _solver.registerInvariantInput(_id, input, true);
  }
  _solver.registerInvariantInput(_id, _index, false);
  registerDefinedVar(_output);
}

void ElementVar::updateBounds(bool widenOnly) {
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

void ElementVar::recompute(Timestamp ts) {
  const size_t index = safeIndex(_solver.value(ts, _index));
  assert(index < _varArray.size());
  for (size_t i = 0; i < _varArray.size(); ++i) {
    if (i != index) {
      makeDynamicInputInactive(ts, LocalId(i));
    }
  }
  makeDynamicInputActive(ts, LocalId(index));
  updateValue(ts, _output, _solver.value(ts, _varArray[index]));
}

VarViewId ElementVar::dynamicInputVar(Timestamp ts) const noexcept {
  return _varArray[safeIndex(_solver.value(ts, _index))];
}

void ElementVar::notifyInputChanged(Timestamp ts, LocalId localId) {
  const size_t index = safeIndex(_solver.value(ts, _index));
  assert(index < _varArray.size());
  if (localId == _varArray.size()) {
    makeDynamicInputInactive(ts, LocalId{safeIndex(_committedIndex)});
    makeDynamicInputActive(ts, LocalId{index});
  }
  updateValue(ts, _output, _solver.value(ts, _varArray[index]));
}

VarViewId ElementVar::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _index;
    case 1: {
      assert(safeIndex(_solver.value(ts, _index)) < _varArray.size());
      return _varArray[safeIndex(_solver.value(ts, _index))];
    }
    default:
      return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentInputChanged(Timestamp ts) {
  assert(safeIndex(_solver.value(ts, _index)) < _varArray.size());
  updateValue(
      ts, _output,
      _solver.value(ts, _varArray[safeIndex(_solver.value(ts, _index))]));
}

void ElementVar::commit(Timestamp ts) {
  Invariant::commit(ts);
  _committedIndex = _solver.committedValue(_index);
}

}  // namespace atlantis::propagation
