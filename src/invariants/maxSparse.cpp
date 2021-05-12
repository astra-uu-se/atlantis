#include "invariants/maxSparse.hpp"

MaxSparse::MaxSparse(std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), _X(X), _b(b), _localPriority(X.size()) {
  _modifiedVars.reserve(_X.size());
}

void MaxSparse::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(e, _b);
  for (size_t i = 0; i < _X.size(); ++i) {
    e.registerInvariantDependsOnVar(_id, _X[i], LocalId(i));
  }
}

void MaxSparse::recompute(Timestamp t, Engine& e) {
  for (size_t i = 0; i < _X.size(); ++i) {
    _localPriority.updatePriority(t, i, e.getValue(t, _X[i]));
  }
  updateValue(t, e, _b, _localPriority.getMaxPriority(t));
}

void MaxSparse::notifyIntChanged(Timestamp t, Engine& e, LocalId i) {
  auto newValue = e.getValue(t, _X[i]);
  _localPriority.updatePriority(t, i, newValue);
  updateValue(t, e, _b, _localPriority.getMaxPriority(t));
}

VarId MaxSparse::getNextDependency(Timestamp t, Engine&) {
  _state.incValue(t, 1);
  if (static_cast<size_t>(_state.getValue(t)) == _X.size()) {
    return NULL_ID;  // Done
  } else {
    return _X.at(_state.getValue(t));
  }
}

void MaxSparse::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  notifyIntChanged(t, e, _state.getValue(t));
}

void MaxSparse::commit(Timestamp t, Engine& e) {
  Invariant::commit(t, e);
  _localPriority.commitIf(t);
}