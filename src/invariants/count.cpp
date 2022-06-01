#include "invariants/count.hpp"

#include <utility>

#include "core/engine.hpp"

Count::Count(VarId output, VarId y, std::vector<VarId> varArray)
    : Invariant(),
      _output(output),
      _y(y),
      _variables(std::move(varArray)),
      _localValues(),
      _counts(),
      _offset(0) {
  _localValues.reserve(_variables.size());
  _modifiedVars.reserve(_variables.size() + 1);
}

void Count::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _variables.size(); ++i) {
    engine.registerInvariantInput(_id, _variables[i], i);
  }
  engine.registerInvariantInput(_id, _y, _variables.size());
  registerDefinedVariable(engine, _output);
}

void Count::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(_output, 0, _variables.size(), widenOnly);
}

void Count::close(Timestamp ts, Engine& engine) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < _variables.size(); ++i) {
    lb = std::min(lb, engine.lowerBound(_variables[i]));
    ub = std::max(ub, engine.upperBound(_variables[i]));
    _localValues.emplace_back(ts, engine.committedValue(_variables[i]));
  }
  assert(ub >= lb);
  lb = std::max(lb, engine.lowerBound(_y));
  ub = std::max(ub, engine.lowerBound(_y));

  _counts.resize(static_cast<unsigned long>(ub - lb + 1),
                 CommittableInt(ts, 0));
  _offset = lb;
}

void Count::recompute(Timestamp ts, Engine& engine) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  updateValue(ts, engine, _output, 0);

  for (size_t i = 0; i < _variables.size(); ++i) {
    _localValues[i].setValue(ts, engine.value(ts, _variables[i]));
    increaseCount(ts, engine.value(ts, _variables[i]));
  }
  updateValue(ts, engine, _output, count(ts, engine.value(ts, _y)));
}

void Count::notifyInputChanged(Timestamp ts, Engine& engine, LocalId id) {
  if (id == _localValues.size()) {
    updateValue(ts, engine, _output, count(ts, engine.value(ts, _y)));
    return;
  }
  assert(id < _localValues.size());
  const Int oldValue = _localValues[id].value(ts);
  const Int newValue = engine.value(ts, _variables[id]);
  if (newValue == oldValue) {
    return;
  }
  decreaseCount(ts, oldValue);
  increaseCount(ts, newValue);
  _localValues[id].setValue(ts, newValue);
  updateValue(ts, engine, _output, count(ts, engine.value(ts, _y)));
}

VarId Count::nextInput(Timestamp ts, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _variables.size()) {
    return _variables[index];
  } else if (index == _variables.size()) {
    return _y;
  }
  return NULL_ID;
}

void Count::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  assert(static_cast<size_t>(_state.value(ts)) <= _variables.size());
  notifyInputChanged(ts, engine, static_cast<size_t>(_state.value(ts)));
}

void Count::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);

  for (CommittableInt& localValue : _localValues) {
    localValue.commitIf(ts);
  }

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}
