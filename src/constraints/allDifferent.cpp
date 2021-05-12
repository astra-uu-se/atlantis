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

void AllDifferent::init(Timestamp ts, Engine& e) {
  assert(!_id.equals(NULL_ID));
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < _variables.size(); ++i) {
    lb = std::min(lb, e.getLowerBound(_variables[i]));
    ub = std::max(ub, e.getUpperBound(_variables[i]));
    e.registerInvariantDependsOnVar(_id, _variables[i], LocalId(i));
    _localValues.emplace_back(ts, e.getCommittedValue(_variables[i]));
  }
  assert(ub >= lb);

  _counts.resize(static_cast<unsigned long>(ub - lb + 1), SavedInt(ts, 0));

  registerDefinedVariable(e, _violationId);

  _offset = lb;
}

void AllDifferent::recompute(Timestamp t, Engine& e) {
  for (SavedInt& c : _counts) {
    c.setValue(t, 0);
  }

  updateValue(t, e, _violationId, 0);

  Int violInc = 0;
  for (size_t i = 0; i < _variables.size(); ++i) {
    violInc += increaseCount(t, e.getValue(t, _variables[i]));
    _localValues[i].setValue(t, e.getValue(t, _variables[i]));
  }
  incValue(t, e, _violationId, violInc);
}

void AllDifferent::notifyIntChanged(Timestamp t, Engine& e, LocalId id) {
  Int oldValue = _localValues.at(id).getValue(t);
  auto newValue = e.getValue(t, _variables[id]);
  if (newValue == oldValue) {
    return;
  }
  signed char dec = decreaseCount(t, oldValue);
  signed char inc = increaseCount(t, newValue);
  _localValues.at(id).setValue(t, newValue);
  incValue(t, e, _violationId, static_cast<Int>(dec + inc));
}

VarId AllDifferent::getNextDependency(Timestamp t, Engine&) {
  _state.incValue(t, 1);

  auto index = static_cast<size_t>(_state.getValue(t));
  if (index < _variables.size()) {
    return _variables.at(index);
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  auto id = static_cast<size_t>(_state.getValue(t));
  assert(id < _variables.size());
  notifyIntChanged(t, e, id);
}

void AllDifferent::commit(Timestamp t, Engine& e) {
  Invariant::commit(t, e);

  for (auto& localValue : _localValues) {
    localValue.commitIf(t);
  }

  for (SavedInt& savedInt : _counts) {
    savedInt.commitIf(t);
  }
}
