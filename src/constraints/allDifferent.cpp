#include "constraints/allDifferent.hpp"

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(Engine& engine, VarId violationId,
                           std::vector<VarId> variables)
    : Constraint(engine, violationId),
      _variables(std::move(variables)),
      _committedValues(_variables.size(), 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_variables.size());
}

void AllDifferent::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _variables.size(); ++i) {
    _engine.registerInvariantInput(_id, _variables[i], i);
  }
  registerDefinedVariable(_violationId);
}

void AllDifferent::updateBounds(bool widenOnly) {
  _engine.updateBounds(_violationId, 0, _variables.size() - 1, widenOnly);
}

void AllDifferent::close(Timestamp ts) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < _variables.size(); ++i) {
    lb = std::min(lb, _engine.lowerBound(_variables[i]));
    ub = std::max(ub, _engine.upperBound(_variables[i]));
  }
  assert(ub >= lb);
  _counts.resize(static_cast<unsigned long>(ub - lb + 1),
                 CommittableInt(ts, 0));
  _offset = lb;
}

void AllDifferent::recompute(Timestamp ts) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  Int violInc = 0;
  for (size_t i = 0; i < _variables.size(); ++i) {
    violInc += increaseCount(ts, _engine.value(ts, _variables[i]));
  }
  updateValue(ts, _violationId, violInc);
}

void AllDifferent::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _committedValues.size());
  const Int newValue = _engine.value(ts, _variables[id]);
  if (newValue == _committedValues[id]) {
    return;
  }
  incValue(ts, _violationId,
           static_cast<Int>(decreaseCount(ts, _committedValues[id]) +
                            increaseCount(ts, newValue)));
}

VarId AllDifferent::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _variables.size()) {
    return _variables[index];
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentInputChanged(Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) < _variables.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void AllDifferent::commit(Timestamp ts) {
  Invariant::commit(ts);

  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _engine.committedValue(_variables[i]);
  }

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}
