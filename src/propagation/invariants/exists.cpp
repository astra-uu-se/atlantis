#include "atlantis/propagation/invariants/exists.hpp"

#include <limits>
#include <utility>
#include <vector>

namespace atlantis::propagation {

Exists::Exists(SolverBase& solver, VarId output, std::vector<VarId>&& varArray)
    : Invariant(solver),
      _output(output),
      _varArray(std::move(varArray)),
      _minIndex(NULL_TIMESTAMP, 0) {}

void Exists::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  registerDefinedVar(_output);
}

void Exists::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::max();
  for (const VarId& input : _varArray) {
    lb = std::min(lb, _solver.lowerBound(input));
    ub = std::min(ub, _solver.upperBound(input));
  }
  _solver.updateBounds(_output, std::max(Int(0), lb), std::max(Int(0), ub),
                       widenOnly);
}

void Exists::recompute(Timestamp ts) {
  Int minIndex = 0;
  Int minVal = _solver.value(ts, _varArray[0]);
  for (size_t i = 1; i < _varArray.size(); ++i) {
    if (_solver.value(ts, _varArray[i]) < minVal) {
      minIndex = static_cast<Int>(i);
      minVal = _solver.value(ts, _varArray[i]);
    }
    assert(minVal >= 0);
    if (minVal == 0) {
      break;
    }
  }
  _minIndex.setValue(ts, minIndex);
  updateValue(ts, _output, minVal);
}

void Exists::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(0 <= _minIndex.value(ts) &&
         _minIndex.value(ts) < static_cast<Int>(_varArray.size()));
  assert(size_t(id) < _varArray.size());
  if (static_cast<size_t>(_minIndex.value(ts)) == id) {
    if (_solver.value(ts, _varArray[id]) >
        _solver.committedValue(_varArray[id])) {
      recompute(ts);
    }
  } else if (_solver.value(ts, _varArray[id]) <=
             _solver.value(ts, _varArray[_minIndex.value(ts)])) {
    if (_solver.value(ts, _output) < _solver.value(ts, _varArray[id])) {
      recompute(ts);
    } else {
      _minIndex.setValue(ts, static_cast<Int>(id));
      updateValue(ts, _output, _solver.value(ts, _varArray[id]));
    }
  }
}

VarId Exists::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index == 0 || (index < _varArray.size() &&
                     _solver.value(ts, _varArray[index - 1]) > 0)) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void Exists::notifyCurrentInputChanged(Timestamp ts) {
  notifyInputChanged(ts, _state.value(ts));
}

void Exists::commit(Timestamp ts) {
  Invariant::commit(ts);
  _minIndex.commitIf(ts);
}
}  // namespace atlantis::propagation
