#include "invariants/linear.hpp"

#include <utility>

#include "core/engine.hpp"

Linear::Linear(std::vector<Int> A, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), _A(std::move(A)), _X(std::move(X)), _localX(), _b(b) {
  _localX.reserve(_X.size());
  _modifiedVars.reserve(_X.size());
}

void Linear::init(Timestamp t, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  registerDefinedVariable(e, _b);
  for (size_t i = 0; i < _X.size(); ++i) {
    e.registerInvariantDependsOnVar(_id, _X[i], LocalId(i));
    _localX.emplace_back(t, e.getCommittedValue(_X[i]));
  }
}

void Linear::recompute(Timestamp t, Engine& e) {
  Int sum = 0;
  for (size_t i = 0; i < _X.size(); ++i) {
    sum += _A[i] * e.getValue(t, _X[i]);
    _localX.at(i).commitValue(e.getCommittedValue(_X[i]));
    _localX.at(i).setValue(t, e.getValue(t, _X[i]));
  }
  updateValue(t, e, _b, sum);
}

void Linear::notifyIntChanged(Timestamp t, Engine& e, LocalId i) {
  auto newValue = e.getValue(t, _X[i]);
  incValue(t, e, _b, (newValue - _localX.at(i).getValue(t)) * _A[i]);
  _localX.at(i).setValue(t, newValue);
}

VarId Linear::getNextDependency(Timestamp t, Engine&) {
  _state.incValue(t, 1);
  if (static_cast<size_t>(_state.getValue(t)) == _X.size()) {
    return NULL_ID;  // Done
  } else {
    return _X.at(_state.getValue(t));
  }
}

void Linear::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(_state.getValue(t) != -1);
  notifyIntChanged(t, e, _state.getValue(t));
}

void Linear::commit(Timestamp t, Engine& e) {
  Invariant::commit(t, e);
  for (size_t i = 0; i < _X.size(); ++i) {
    _localX.at(i).commitIf(t);
  }
}
