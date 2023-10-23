#include "propagation/invariants/maxSparse.hpp"

namespace atlantis::propagation {

MaxSparse::MaxSparse(Engine& engine, VarId output, std::vector<VarId> varArray)
    : Invariant(engine),
      _output(output),
      _varArray(std::move(varArray)),
      _localPriority(_varArray.size()) {
  assert(!_varArray.empty());
  _modifiedVars.reserve(_varArray.size());
}

void MaxSparse::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _engine.registerInvariantInput(_id, _varArray[i], i);
  }
  registerDefinedVariable(_output);
}

void MaxSparse::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();
  for (const VarId input : _varArray) {
    lb = std::max(lb, _engine.lowerBound(input));
    ub = std::max(ub, _engine.upperBound(input));
  }
  _engine.updateBounds(_output, lb, ub, widenOnly);
}

void MaxSparse::recompute(Timestamp ts) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, _engine.value(ts, _varArray[i]));
  }
  updateValue(ts, _output, _localPriority.maxPriority(ts));
}

void MaxSparse::notifyInputChanged(Timestamp ts, LocalId id) {
  _localPriority.updatePriority(ts, id, _engine.value(ts, _varArray[id]));
  updateValue(ts, _output, _localPriority.maxPriority(ts));
}

VarId MaxSparse::nextInput(Timestamp ts) {
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