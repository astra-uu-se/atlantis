#include "invariants/minSparse.hpp"

MinSparse::MinSparse(std::vector<VarId> varArray, VarId y)
    : Invariant(NULL_ID),
      _varArray(varArray),
      _y(y),
      _localPriority(varArray.size()) {
  _modifiedVars.reserve(_varArray.size());
}

void MinSparse::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _y);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    engine.registerInvariantParameter(_id, _varArray[i], LocalId(i));
  }
}

void MinSparse::recompute(Timestamp ts, Engine& engine) {
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _localPriority.updatePriority(ts, i, engine.value(ts, _varArray[i]));
  }
  updateValue(ts, engine, _y, _localPriority.minPriority(ts));
}

void MinSparse::notifyIntChanged(Timestamp ts, Engine& engine, LocalId id) {
  auto newValue = engine.value(ts, _varArray[id]);
  _localPriority.updatePriority(ts, id, newValue);
  updateValue(ts, engine, _y, _localPriority.minPriority(ts));
}

VarId MinSparse::nextParameter(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  if (static_cast<size_t>(_state.value(ts)) == _varArray.size()) {
    return NULL_ID;  // Done
  } else {
    return _varArray.at(_state.value(ts));
  }
}

void MinSparse::notifyCurrentParameterChanged(Timestamp ts, Engine& engine) {
  notifyIntChanged(ts, engine, _state.value(ts));
}

void MinSparse::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  _localPriority.commitIf(ts);
}