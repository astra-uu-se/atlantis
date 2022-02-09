#include "invariants/linear.hpp"

#include <utility>

#include "core/engine.hpp"

Linear::Linear(std::vector<Int> A, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID),
      _coeffs(std::move(A)),
      _varArray(std::move(X)),
      _localVarArray(),
      _y(b) {
  _localVarArray.reserve(_varArray.size());
  _modifiedVars.reserve(_varArray.size());
}

void Linear::init(Timestamp ts, Engine& engine) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  registerDefinedVariable(engine, _y);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    engine.registerInvariantInput(_id, _varArray[i], i);
    _localVarArray.emplace_back(ts, engine.getCommittedValue(_varArray[i]));
  }
}

void Linear::recompute(Timestamp ts, Engine& engine) {
  Int sum = 0;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    sum += _coeffs[i] * engine.getValue(ts, _varArray[i]);
    _localVarArray[i].commitValue(engine.getCommittedValue(_varArray[i]));
    _localVarArray[i].setValue(ts, engine.getValue(ts, _varArray[i]));
  }
  updateValue(ts, engine, _y, sum);
}

void Linear::notifyIntChanged(Timestamp ts, Engine& engine, LocalId id) {
  assert(0 <= id && id < _localVarArray.size());
  const Int newValue = engine.getValue(ts, _varArray[id]);
  incValue(ts, engine, _y,
           (newValue - _localVarArray[id].getValue(ts)) * _coeffs[id]);
  _localVarArray[id].setValue(ts, newValue);
}

VarId Linear::getNextInput(Timestamp ts, Engine&) {
  const size_t index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.getValue(ts));
  if (index < _varArray.size()) {
    return _varArray[_state.getValue(ts)];
  }
  return NULL_ID;  // Done
}

void Linear::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  assert(_state.getValue(ts) != -1);
  notifyIntChanged(ts, engine, _state.getValue(ts));
}

void Linear::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  for (size_t i = 0; i < _localVarArray.size(); ++i) {
    _localVarArray[i].commitIf(ts);
  }
}
