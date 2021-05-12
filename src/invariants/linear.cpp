#include "invariants/linear.hpp"

#include <utility>

#include "core/engine.hpp"

Linear::Linear(std::vector<Int> A, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), _A(std::move(A)), _X(std::move(X)), _localX(), _b(b) {
  _localX.reserve(_X.size());
  _modifiedVars.reserve(_X.size());
}

void Linear::init(Timestamp ts, Engine& engine) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  registerDefinedVariable(engine, _b);
  for (size_t i = 0; i < _X.size(); ++i) {
    engine.registerInvariantDependsOnVar(_id, _X[i], LocalId(i));
    _localX.emplace_back(ts, engine.getCommittedValue(_X[i]));
  }
}

void Linear::recompute(Timestamp ts, Engine& engine) {
  Int sum = 0;
  for (size_t i = 0; i < _X.size(); ++i) {
    sum += _A[i] * engine.getValue(ts, _X[i]);
    _localX.at(i).commitValue(engine.getCommittedValue(_X[i]));
    _localX.at(i).setValue(ts, engine.getValue(ts, _X[i]));
  }
  updateValue(ts, engine, _b, sum);
}

void Linear::notifyIntChanged(Timestamp ts, Engine& engine, LocalId id) {
  auto newValue = engine.getValue(ts, _X[id]);
  incValue(ts, engine, _b, (newValue - _localX.at(id).getValue(ts)) * _A[id]);
  _localX.at(i).setValue(ts, newValue);
}

VarId Linear::getNextDependency(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  if (static_cast<size_t>(_state.getValue(ts)) == _X.size()) {
    return NULL_ID;  // Done
  } else {
    return _X.at(_state.getValue(ts));
  }
}

void Linear::notifyCurrentDependencyChanged(Timestamp ts, Engine& engine) {
  assert(_state.getValue(ts) != -1);
  notifyIntChanged(ts, engine, _state.getValue(ts));
}

void Linear::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  for (size_t i = 0; i < _X.size(); ++i) {
    _localX.at(i).commitIf(ts);
  }
}
