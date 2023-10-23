#include "propagation/invariants/count.hpp"

namespace atlantis::propagation {

Count::Count(Engine& engine, VarId output, VarId y, std::vector<VarId> varArray)
    : Invariant(engine),
      _output(output),
      _y(y),
      _variables(std::move(varArray)),
      _committedValues(_variables.size(), 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_variables.size() + 1);
}

void Count::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _variables.size(); ++i) {
    _engine.registerInvariantInput(_id, _variables[i], i);
  }
  _engine.registerInvariantInput(_id, _y, _variables.size());
  registerDefinedVariable(_output);
}

void Count::updateBounds(bool widenOnly) {
  _engine.updateBounds(_output, 0, _variables.size(), widenOnly);
}

void Count::close(Timestamp ts) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < _variables.size(); ++i) {
    lb = std::min(lb, _engine.lowerBound(_variables[i]));
    ub = std::max(ub, _engine.upperBound(_variables[i]));
  }
  assert(ub >= lb);
  lb = std::max(lb, _engine.lowerBound(_y));
  ub = std::max(ub, _engine.lowerBound(_y));

  _counts.resize(static_cast<unsigned long>(ub - lb + 1),
                 CommittableInt(ts, 0));
  _offset = lb;
}

void Count::recompute(Timestamp ts) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  updateValue(ts, _output, 0);

  for (size_t i = 0; i < _variables.size(); ++i) {
    increaseCount(ts, _engine.value(ts, _variables[i]));
  }
  updateValue(ts, _output, count(ts, _engine.value(ts, _y)));
}

void Count::notifyInputChanged(Timestamp ts, LocalId id) {
  if (id == _committedValues.size()) {
    updateValue(ts, _output, count(ts, _engine.value(ts, _y)));
    return;
  }
  assert(id < _committedValues.size());
  const Int newValue = _engine.value(ts, _variables[id]);
  if (newValue == _committedValues[id]) {
    return;
  }
  decreaseCount(ts, _committedValues[id]);
  increaseCount(ts, newValue);
  updateValue(ts, _output, count(ts, _engine.value(ts, _y)));
}

VarId Count::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _variables.size()) {
    return _variables[index];
  } else if (index == _variables.size()) {
    return _y;
  }
  return NULL_ID;
}

void Count::notifyCurrentInputChanged(Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) <= _variables.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void Count::commit(Timestamp ts) {
  Invariant::commit(ts);

  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _engine.committedValue(_variables[i]);
  }

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}
}  // namespace atlantis::propagation