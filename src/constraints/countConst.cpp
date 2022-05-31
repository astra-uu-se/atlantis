#include "constraints/countConst.hpp"

#include <utility>

#include "core/engine.hpp"

CountConst::CountConst(VarId violationId, Int y, std::vector<VarId> variables)
    : Constraint(violationId), _y(y), _variables(std::move(variables)) {
  _hasCountValue.reserve(_variables.size());
  _modifiedVars.reserve(_variables.size());
}

void CountConst::registerVars(Engine& engine) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _variables.size(); ++i) {
    engine.registerInvariantInput(_id, _variables[i], i);
  }
  registerDefinedVariable(engine, _violationId);
}

void CountConst::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(_violationId, 0, _variables.size(), widenOnly);
}

void CountConst::close(Timestamp ts, Engine& engine) {
  _hasCountValue.clear();
  for (const VarId input : _variables) {
    _hasCountValue.emplace_back(
        ts, static_cast<Int>(engine.committedValue(input) == _y));
  }
}

void CountConst::recompute(Timestamp ts, Engine& engine) {
  Int count = 0;
  for (size_t i = 0; i < _variables.size(); ++i) {
    _hasCountValue[i].commitValue(
        static_cast<Int>(engine.committedValue(_variables[i])) == _y);
    count += static_cast<Int>(engine.value(ts, _variables[i]) == _y);
    _hasCountValue[i].setValue(
        ts, static_cast<Int>(engine.value(ts, _variables[i])) == _y);
  }
  updateValue(ts, engine, _violationId, count);
}

void CountConst::notifyInputChanged(Timestamp ts, Engine& engine, LocalId id) {
  assert(id < _hasCountValue.size());
  const Int oldValue = _hasCountValue[id].value(ts);
  const Int newValue = static_cast<Int>(engine.value(ts, _variables[id]) == _y);
  if (oldValue == newValue) {
    return;
  }
  _hasCountValue[id].setValue(ts, newValue);
  incValue(ts, engine, _violationId, newValue - oldValue);
}

VarId CountConst::nextInput(Timestamp ts, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _variables.size()) {
    return _variables[index];
  }
  return NULL_ID;  // Done
}

void CountConst::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, engine, _state.value(ts));
}

void CountConst::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  for (size_t i = 0; i < _hasCountValue.size(); ++i) {
    _hasCountValue[i].commitIf(ts);
    assert(_hasCountValue[i].committedValue() ==
           static_cast<Int>(engine.committedValue(_variables[i]) == _y));
  }
}
