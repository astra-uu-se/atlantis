#include "invariants/maxSparse.hpp"

MaxSparse::MaxSparse(std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), _X(X), _b(b), _localPriority(X.size()) {
  _modifiedVars.reserve(_X.size());
}

void MaxSparse::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _b);
  for (size_t i = 0; i < _X.size(); ++i) {
    engine.registerInvariantDependsOnVar(_id, _X[i], LocalId(i));
  }
}

void MaxSparse::recompute(Timestamp ts, Engine& engine) {
  for (size_t i = 0; i < _X.size(); ++i) {
    _localPriority.updatePriority(ts, i, engine.getValue(ts, _X[i]));
  }
  updateValue(ts, engine, _b, _localPriority.getMaxPriority(ts));
}

void MaxSparse::notifyIntChanged(Timestamp ts, Engine& engine, LocalId id) {
  auto newValue = engine.getValue(ts, _X[id]);
  _localPriority.updatePriority(ts, id, newValue);
  updateValue(ts, engine, _b, _localPriority.getMaxPriority(ts));
}

VarId MaxSparse::getNextDependency(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  if (static_cast<size_t>(_state.getValue(ts)) == _X.size()) {
    return NULL_ID;  // Done
  } else {
    return _X.at(_state.getValue(ts));
  }
}

void MaxSparse::notifyCurrentDependencyChanged(Timestamp ts, Engine& engine) {
  notifyIntChanged(ts, engine, _state.getValue(ts));
}

void MaxSparse::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  _localPriority.commitIf(ts);
}