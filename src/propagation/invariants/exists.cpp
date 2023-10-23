#include "propagation/invariants/exists.hpp"

namespace atlantis::propagation {

Exists::Exists(Engine& engine, VarId output, std::vector<VarId> varArray)
    : Invariant(engine),
      _output(output),
      _varArray(std::move(varArray)),
      _min(NULL_TIMESTAMP, 0) {
  _modifiedVars.reserve(_varArray.size());
}

void Exists::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _engine.registerInvariantInput(_id, _varArray[i], i);
  }
  registerDefinedVariable(_output);
}

void Exists::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::max();
  for (const VarId input : _varArray) {
    lb = std::min(lb, _engine.lowerBound(input));
    ub = std::min(ub, _engine.upperBound(input));
  }
  _engine.updateBounds(_output, std::max(Int(0), lb), ub, widenOnly);
}

void Exists::recompute(Timestamp ts) {
  Int min_i = 0;
  Int min_val = _engine.value(ts, _varArray[0]);
  for (size_t i = 1; i < _varArray.size(); ++i) {
    if (_engine.value(ts, _varArray[i]) < min_val) {
      min_i = i;
      min_val = _engine.value(ts, _varArray[i]);
    }
    assert(min_val >= 0);
    if (min_val == 0) {
      break;
    }
  }
  _min.setValue(ts, min_i);
  updateValue(ts, _output, min_val);
}

void Exists::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(0 <= _min.value(ts) &&
         _min.value(ts) < static_cast<Int>(_varArray.size()));
  if (static_cast<size_t>(_min.value(ts)) == id) {
    recompute(ts);
  } else if (_engine.value(ts, _varArray[id]) <=
             _engine.value(ts, _varArray[_min.value(ts)])) {
    _min.setValue(ts, id);
    updateValue(ts, _output, _engine.value(ts, _varArray[_min.value(ts)]));
  }
}

VarId Exists::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index == 0) {
    return _varArray[index];
  } else if (index < _varArray.size() &&
             _engine.value(ts, _varArray[index - 1]) > 0) {
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
  _min.commitIf(ts);
}
}  // namespace atlantis::propagation