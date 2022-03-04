#include "invariants/maxSparse.hpp"

MaxSparse::MaxSparse(std::vector<VarId> varArray, VarId y)
    : Invariant(NULL_ID),
      _varArray(std::move(varArray)),
      _y(y),
      _localPriority(_varArray.size()) {
  _modifiedVars.reserve(_varArray.size());
}

void MaxSparse::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _y);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    engine.registerInvariantInput(_id, _varArray[i], i);
  }
}

void MaxSparse::recompute(Timestamp ts, Engine& engine) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, engine.value(ts, _varArray[i]));
  }
  updateValue(ts, engine, _y, _localPriority.maxPriority(ts));
}

void MaxSparse::notifyInputChanged(Timestamp ts, Engine& engine, LocalId id) {
  _localPriority.updatePriority(ts, id, engine.value(ts, _varArray[id]));
  updateValue(ts, engine, _y, _localPriority.maxPriority(ts));
}

VarId MaxSparse::nextInput(Timestamp ts, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void MaxSparse::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  notifyInputChanged(ts, engine, _state.value(ts));
}

void MaxSparse::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  _localPriority.commitIf(ts);
}