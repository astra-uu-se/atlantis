#include "invariants/boolLinear.hpp"

#include <utility>

#include "core/engine.hpp"

BoolLinear::BoolLinear(VarId output, const std::vector<VarId>& violArray)
    : BoolLinear(output, std::vector<Int>(violArray.size(), 1), violArray) {}

BoolLinear::BoolLinear(VarId output, std::vector<Int> coeffs,
                       std::vector<VarId> violArray)
    : Invariant(),
      _output(output),
      _coeffs(std::move(coeffs)),
      _violArray(std::move(violArray)),
      _isSatisfied() {
  _isSatisfied.reserve(_violArray.size());
  _modifiedVars.reserve(_violArray.size());
}

void BoolLinear::registerVars(Engine& engine) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _violArray.size(); ++i) {
    engine.registerInvariantInput(_id, _violArray[i], i);
  }
  registerDefinedVariable(engine, _output);
}

void BoolLinear::updateBounds(Engine& engine, bool widenOnly) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < _violArray.size(); ++i) {
    const Int violLb = engine.lowerBound(_violArray[i]);
    const Int violUb = engine.upperBound(_violArray[i]);
    // violation != 0 <=> false
    const Int boolLb = static_cast<Int>(violLb == 0 && violUb == 0);
    // violation == 0 <=> true
    const Int boolUb = static_cast<Int>(violLb <= 0 && 0 <= violUb);
    assert(0 <= boolLb);
    assert(boolLb <= boolUb);
    assert(boolUb <= 1);

    lb += _coeffs[i] * (_coeffs[i] < 0 ? boolUb : boolLb);
    ub += _coeffs[i] * (_coeffs[i] < 0 ? boolLb : boolUb);
  }
  engine.updateBounds(_output, lb, ub, widenOnly);
}

void BoolLinear::close(Timestamp ts, Engine& engine) {
  _isSatisfied.clear();
  for (const VarId input : _violArray) {
    _isSatisfied.emplace_back(ts, engine.committedValue(input));
  }
}

void BoolLinear::recompute(Timestamp ts, Engine& engine) {
  Int sum = 0;
  for (size_t i = 0; i < _violArray.size(); ++i) {
    sum += _coeffs[i] * static_cast<Int>(engine.value(ts, _violArray[i]) == 0);
    _isSatisfied[i].commitValue(engine.committedValue(_violArray[i]) == 0);
    _isSatisfied[i].setValue(ts, engine.value(ts, _violArray[i]) == 0);
  }
  updateValue(ts, engine, _output, sum);
}

void BoolLinear::notifyInputChanged(Timestamp ts, Engine& engine, LocalId id) {
  assert(id < _isSatisfied.size());
  const Int newValue = static_cast<Int>(engine.value(ts, _violArray[id]) == 0);
  if (newValue == _isSatisfied[id].value(ts)) {
    return;
  }
  incValue(ts, engine, _output,
           (newValue - _isSatisfied[id].value(ts)) * _coeffs[id]);
  _isSatisfied[id].setValue(ts, newValue);
}

VarId BoolLinear::nextInput(Timestamp ts, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _violArray.size()) {
    return _violArray[index];
  }
  return NULL_ID;  // Done
}

void BoolLinear::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, engine, _state.value(ts));
}

void BoolLinear::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  for (size_t i = 0; i < _isSatisfied.size(); ++i) {
    _isSatisfied[i].commitIf(ts);
    assert(static_cast<Int>(engine.committedValue(_violArray[i]) == 0) ==
           _isSatisfied[i].committedValue());
  }
}
