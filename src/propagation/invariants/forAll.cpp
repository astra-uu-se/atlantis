#include "propagation/invariants/forAll.hpp"

namespace atlantis::propagation {

ForAll::ForAll(Engine& engine, VarId output, std::vector<VarId> varArray)
    : Invariant(engine),
      _output(output),
      _varArray(std::move(varArray)),
      _localPriority(_varArray.size()) {
  _modifiedVars.reserve(_varArray.size());
}

void ForAll::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _engine.registerInvariantInput(_id, _varArray[i], i);
  }
  registerDefinedVariable(_output);
}

void ForAll::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();
  for (const VarId input : _varArray) {
    lb = std::max(lb, _engine.lowerBound(input));
    ub = std::max(ub, _engine.upperBound(input));
  }
  _engine.updateBounds(_output, std::max(Int(0), lb), ub, widenOnly);
}

void ForAll::recompute(Timestamp ts) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, _engine.value(ts, _varArray[i]));
  }
  assert(_localPriority.minPriority(ts) >= 0);
  updateValue(ts, _output, _localPriority.maxPriority(ts));
}

void ForAll::notifyInputChanged(Timestamp ts, LocalId id) {
  _localPriority.updatePriority(
      ts, id, std::max(Int(0), _engine.value(ts, _varArray[id])));
  assert(_localPriority.minPriority(ts) >= 0);
  updateValue(ts, _output, _localPriority.maxPriority(ts));
}

VarId ForAll::nextInput(Timestamp ts) {
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