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
    _localPriority.updatePriority(ts, i, engine.getValue(ts, _varArray[i]));
  }
  updateValue(ts, engine, _y, _localPriority.getMaxPriority(ts));
}

void MaxSparse::notifyIntChanged(Timestamp ts, Engine& engine, LocalId id) {
  _localPriority.updatePriority(ts, id, engine.getValue(ts, _varArray[id]));
  updateValue(ts, engine, _y, _localPriority.getMaxPriority(ts));
}

VarId MaxSparse::getNextInput(Timestamp ts, Engine&) {
  const size_t index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.getValue(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void MaxSparse::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  notifyIntChanged(ts, engine, _state.getValue(ts));
}

void MaxSparse::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  _localPriority.commitIf(ts);
}