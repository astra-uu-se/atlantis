#include "invariants/linear.hpp"

Linear::Linear(Engine& engine, VarId output, const std::vector<VarId>& varArray)
    : Linear(engine, output, std::vector<Int>(varArray.size(), 1), varArray) {}

Linear::Linear(Engine& engine, VarId output, std::vector<Int> coeffs,
               std::vector<VarId> varArray)
    : Invariant(engine),
      _output(output),
      _coeffs(std::move(coeffs)),
      _varArray(std::move(varArray)),
      _committedValues(_varArray.size(), 0) {
  _modifiedVars.reserve(_varArray.size());
}

void Linear::registerVars() {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _varArray.size(); ++i) {
    _engine.registerInvariantInput(_id, _varArray[i], i);
  }
  registerDefinedVariable(_output);
}

void Linear::updateBounds(bool widenOnly) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    lb += _coeffs[i] * (_coeffs[i] < 0 ? _engine.upperBound(_varArray[i])
                                       : _engine.lowerBound(_varArray[i]));
    ub += _coeffs[i] * (_coeffs[i] < 0 ? _engine.lowerBound(_varArray[i])
                                       : _engine.upperBound(_varArray[i]));
  }
  _engine.updateBounds(_output, lb, ub, widenOnly);
}

void Linear::recompute(Timestamp ts) {
  Int sum = 0;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    sum += _coeffs[i] * _engine.value(ts, _varArray[i]);
  }
  updateValue(ts, _output, sum);
}

void Linear::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _committedValues.size());
  const Int newValue = _engine.value(ts, _varArray[id]);
  incValue(ts, _output, (newValue - _committedValues[id]) * _coeffs[id]);
}

VarId Linear::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  }
  return NULL_ID;  // Done
}

void Linear::notifyCurrentInputChanged(Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

void Linear::commit(Timestamp ts) {
  Invariant::commit(ts);
  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _engine.committedValue(_varArray[i]);
  }
}
