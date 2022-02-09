#include "constraints/allDifferent.hpp"

#include "core/engine.hpp"
#include "variables/savedInt.hpp"
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

void AllDifferent::init(Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < _variables.size(); ++i) {
    lb = std::min(lb, engine.getLowerBound(_variables[i]));
    ub = std::max(ub, engine.getUpperBound(_variables[i]));
    engine.registerInvariantInput(_id, _variables[i], i);
    _localValues.emplace_back(ts, engine.getCommittedValue(_variables[i]));
  }
  assert(ub >= lb);

  _counts.resize(static_cast<unsigned long>(ub - lb + 1), SavedInt(ts, 0));

  registerDefinedVariable(engine, _violationId);

  _offset = lb;
}

void AllDifferent::recompute(Timestamp ts, Engine& engine) {
  for (SavedInt& c : _counts) {
    c.setValue(ts, 0);
  }

  updateValue(ts, engine, _violationId, 0);

  Int violInc = 0;
  for (size_t i = 0; i < _variables.size(); ++i) {
    violInc += increaseCount(ts, engine.getValue(ts, _variables[i]));
    _localValues[i].setValue(ts, engine.getValue(ts, _variables[i]));
  }
  incValue(ts, engine, _violationId, violInc);
}

void AllDifferent::notifyIntChanged(Timestamp ts, Engine& engine, LocalId id) {
  assert(id < _localValues.size());
  const Int oldValue = _localValues[id].getValue(ts);
  const Int newValue = engine.getValue(ts, _variables[id]);
  if (newValue == oldValue) {
    return;
  }
  _localValues[id].setValue(ts, newValue);
  incValue(ts, engine, _violationId,
           static_cast<Int>(decreaseCount(ts, oldValue) +
                            increaseCount(ts, newValue)));
}

VarId AllDifferent::getNextInput(Timestamp ts, Engine&) {
  const size_t index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _variables.size()) {
    return _variables[index];
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  assert(static_cast<size_t>(_state.getValue(ts)) < _variables.size());
  notifyIntChanged(ts, engine, static_cast<size_t>(_state.getValue(ts)));
}

void AllDifferent::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);

  for (SavedInt& localValue : _localValues) {
    localValue.commitIf(ts);
  }

  for (SavedInt& savedInt : _counts) {
    savedInt.commitIf(ts);
  }
}
