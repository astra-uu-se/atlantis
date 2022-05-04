#include "invariants/linear.hpp"

#include <utility>

#include "core/engine.hpp"

Linear::Linear(std::vector<Int> A, std::vector<VarId> X, VarId y)
    : Invariant(NULL_ID),
      _coeffs(std::move(A)),
      _varArray(std::move(X)),
      _localVarArray(),
      _y(y) {
  _localVarArray.reserve(_varArray.size());
  _modifiedVars.reserve(_varArray.size());
}

void Linear::registerVars(Engine& engine) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _varArray.size(); ++i) {
    engine.registerInvariantInput(_id, _varArray[i], i);
  }
  registerDefinedVariable(engine, _y);
}

void Linear::updateBounds(Engine& engine, bool widenOnly) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    lb += _coeffs[i] * (_coeffs[i] < 0 ? engine.upperBound(_varArray[i])
                                       : engine.lowerBound(_varArray[i]));
    ub += _coeffs[i] * (_coeffs[i] < 0 ? engine.lowerBound(_varArray[i])
                                       : engine.upperBound(_varArray[i]));
  }
  engine.updateBounds(_y, lb, ub, widenOnly);
}

void Linear::close(Timestamp ts, Engine& engine) {
  _localVarArray.clear();
  for (const VarId input : _varArray) {
    _localVarArray.emplace_back(ts, engine.committedValue(input));
  }
}

void Linear::recompute(Timestamp ts, Engine& engine) {
  Int sum = 0;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    sum += _coeffs[i] * engine.value(ts, _varArray[i]);
    _localVarArray[i].commitValue(engine.committedValue(_varArray[i]));
    _localVarArray[i].setValue(ts, engine.value(ts, _varArray[i]));
  }
  updateValue(ts, engine, _y, sum);
}

void Linear::notifyInputChanged(Timestamp ts, Engine& engine, LocalId id) {
  assert(id < _localVarArray.size());
  const Int newValue = engine.value(ts, _varArray[id]);
  incValue(ts, engine, _y,
           (newValue - _localVarArray[id].value(ts)) * _coeffs[id]);
  _localVarArray[id].setValue(ts, newValue);
}

VarId Linear::nextInput(Timestamp ts, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  }
  return NULL_ID;  // Done
}

void Linear::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, engine, _state.value(ts));
}

void Linear::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  for (size_t i = 0; i < _localVarArray.size(); ++i) {
    _localVarArray[i].commitIf(ts);
    assert(engine.committedValue(_varArray[i]) ==
           _localVarArray[i].committedValue());
  }
}
