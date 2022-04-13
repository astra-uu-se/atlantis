#include "constraints/allDifferent.hpp"

#include <limits>

#include "core/engine.hpp"
#include "variables/committableInt.hpp"

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(VarId violationId, std::vector<VarId> variables)
    : Constraint(NULL_ID, violationId),
      _variables(std::move(variables)),
      _localValues(),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_variables.size());
}

void AllDifferent::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _variables.size(); ++i) {
    engine.registerInvariantInput(_id, _variables[i], i);
  }
  registerDefinedVariable(engine, _violationId);
}

void AllDifferent::updateBounds(Engine& engine) {
  engine.updateBounds(_violationId, 0, _variables.size() - 1);
}

void AllDifferent::close(Timestamp ts, Engine& engine) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < _variables.size(); ++i) {
    lb = std::min(lb, engine.lowerBound(_variables[i]));
    ub = std::max(ub, engine.upperBound(_variables[i]));
    _localValues.emplace_back(ts, engine.committedValue(_variables[i]));
  }
  assert(ub >= lb);
  _counts.resize(static_cast<unsigned long>(ub - lb + 1),
                 CommittableInt(ts, 0));
  _offset = lb;
}

void AllDifferent::recompute(Timestamp ts, Engine& engine) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  updateValue(ts, engine, _violationId, 0);

  Int violInc = 0;
  for (size_t i = 0; i < _variables.size(); ++i) {
    violInc += increaseCount(ts, engine.value(ts, _variables[i]));
    _localValues[i].setValue(ts, engine.value(ts, _variables[i]));
  }
  incValue(ts, engine, _violationId, violInc);
}

void AllDifferent::notifyInputChanged(Timestamp ts, Engine& engine,
                                      LocalId id) {
  assert(id < _localValues.size());
  const Int oldValue = _localValues[id].value(ts);
  const Int newValue = engine.value(ts, _variables[id]);
  if (newValue == oldValue) {
    return;
  }
  _localValues[id].setValue(ts, newValue);
  incValue(ts, engine, _violationId,
           static_cast<Int>(decreaseCount(ts, oldValue) +
                            increaseCount(ts, newValue)));
}

VarId AllDifferent::nextInput(Timestamp ts, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _variables.size()) {
    return _variables[index];
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  assert(static_cast<size_t>(_state.value(ts)) < _variables.size());
  notifyInputChanged(ts, engine, static_cast<size_t>(_state.value(ts)));
}

void AllDifferent::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);

  for (CommittableInt& localValue : _localValues) {
    localValue.commitIf(ts);
  }

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}
