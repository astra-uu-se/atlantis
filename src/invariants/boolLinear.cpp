#include "invariants/boolLinear.hpp"

#include "core/engine.hpp"

BoolLinear::BoolLinear(Engine& engine, VarId output,
                       const std::vector<VarId>& violArray)
    : BoolLinear(engine, output, std::vector<Int>(violArray.size(), 1),
                 violArray) {}

BoolLinear::BoolLinear(Engine& engine, VarId output, std::vector<Int> coeffs,
                       std::vector<VarId> violArray)
    : Invariant(engine),
      _output(output),
      _coeffs(std::move(coeffs)),
      _violArray(std::move(violArray)),
      _isSatisfied() {
  _isSatisfied.reserve(_violArray.size());
  _modifiedVars.reserve(_violArray.size());
}

void BoolLinear::registerVars() {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _violArray.size(); ++i) {
    _engine.registerInvariantInput(_id, _violArray[i], i);
  }
  registerDefinedVariable(_output);
}

void BoolLinear::updateBounds(bool widenOnly) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < _violArray.size(); ++i) {
    const Int violLb = _engine.lowerBound(_violArray[i]);
    const Int violUb = _engine.upperBound(_violArray[i]);
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
  _engine.updateBounds(_output, lb, ub, widenOnly);
}

void BoolLinear::close(Timestamp ts) {
  _isSatisfied.clear();
  for (const VarId input : _violArray) {
    _isSatisfied.emplace_back(ts, _engine.committedValue(input));
  }
}

void BoolLinear::recompute(Timestamp ts) {
  Int sum = 0;
  for (size_t i = 0; i < _violArray.size(); ++i) {
    sum += _coeffs[i] * static_cast<Int>(_engine.value(ts, _violArray[i]) == 0);
    _isSatisfied[i].commitValue(_engine.committedValue(_violArray[i]) == 0);
    _isSatisfied[i].setValue(ts, _engine.value(ts, _violArray[i]) == 0);
  }
  updateValue(ts, _output, sum);
}

void BoolLinear::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _isSatisfied.size());
  const Int newValue = static_cast<Int>(_engine.value(ts, _violArray[id]) == 0);
  if (newValue == _isSatisfied[id].value(ts)) {
    return;
  }
  incValue(ts, _output, (newValue - _isSatisfied[id].value(ts)) * _coeffs[id]);
  _isSatisfied[id].setValue(ts, newValue);
}

VarId BoolLinear::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _violArray.size()) {
    return _violArray[index];
  }
  return NULL_ID;  // Done
}

void BoolLinear::notifyCurrentInputChanged(Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

void BoolLinear::commit(Timestamp ts) {
  Invariant::commit(ts);
  for (size_t i = 0; i < _isSatisfied.size(); ++i) {
    _isSatisfied[i].commitIf(ts);
    assert(static_cast<Int>(_engine.committedValue(_violArray[i]) == 0) ==
           _isSatisfied[i].committedValue());
  }
}
