#include "invariants/countConst.hpp"

#include "core/engine.hpp"

CountConst::CountConst(Engine& engine, VarId output, Int y,
                       std::vector<VarId> variables)
    : Invariant(engine),
      _output(output),
      _y(y),
      _variables(std::move(variables)) {
  _hasCountValue.reserve(_variables.size());
  _modifiedVars.reserve(_variables.size());
}

void CountConst::registerVars() {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _variables.size(); ++i) {
    _engine.registerInvariantInput(_id, _variables[i], i);
  }
  registerDefinedVariable(_output);
}

void CountConst::updateBounds(bool widenOnly) {
  _engine.updateBounds(_output, 0, _variables.size(), widenOnly);
}

void CountConst::close(Timestamp ts) {
  _hasCountValue.clear();
  for (const VarId input : _variables) {
    _hasCountValue.emplace_back(
        ts, static_cast<Int>(_engine.committedValue(input) == _y));
  }
}

void CountConst::recompute(Timestamp ts) {
  Int count = 0;
  for (size_t i = 0; i < _variables.size(); ++i) {
    _hasCountValue[i].commitValue(
        static_cast<Int>(_engine.committedValue(_variables[i])) == _y);
    count += static_cast<Int>(_engine.value(ts, _variables[i]) == _y);
    _hasCountValue[i].setValue(
        ts, static_cast<Int>(_engine.value(ts, _variables[i])) == _y);
  }
  updateValue(ts, _output, count);
}

void CountConst::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _hasCountValue.size());
  const Int oldValue = _hasCountValue[id].value(ts);
  const Int newValue =
      static_cast<Int>(_engine.value(ts, _variables[id]) == _y);
  if (oldValue == newValue) {
    return;
  }
  _hasCountValue[id].setValue(ts, newValue);
  incValue(ts, _output, newValue - oldValue);
}

VarId CountConst::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _variables.size()) {
    return _variables[index];
  }
  return NULL_ID;  // Done
}

void CountConst::notifyCurrentInputChanged(Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

void CountConst::commit(Timestamp ts) {
  Invariant::commit(ts);
  for (size_t i = 0; i < _hasCountValue.size(); ++i) {
    _hasCountValue[i].commitIf(ts);
    assert(_hasCountValue[i].committedValue() ==
           static_cast<Int>(_engine.committedValue(_variables[i]) == _y));
  }
}
