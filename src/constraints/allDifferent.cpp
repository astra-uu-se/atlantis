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
    lb = std::min(lb, engine.lowerBound(_variables[i]));
    ub = std::max(ub, engine.upperBound(_variables[i]));
    engine.registerInvariantParameter(_id, _variables[i], LocalId(i));
    _localValues.emplace_back(ts, engine.committedValue(_variables[i]));
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
    violInc += increaseCount(ts, engine.value(ts, _variables[i]));
    _localValues[i].setValue(ts, engine.value(ts, _variables[i]));
  }
  incValue(ts, engine, _violationId, violInc);
}

void AllDifferent::notifyIntChanged(Timestamp ts, Engine& engine, LocalId id) {
  Int oldValue = _localValues.at(id).value(ts);
  auto newValue = engine.value(ts, _variables[id]);
  if (newValue == oldValue) {
    return;
  }
  signed char dec = decreaseCount(ts, oldValue);
  signed char inc = increaseCount(ts, newValue);
  _localValues.at(id).setValue(ts, newValue);
  incValue(ts, engine, _violationId, static_cast<Int>(dec + inc));
}

VarId AllDifferent::nextParameter(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);

  auto index = static_cast<size_t>(_state.value(ts));
  if (index < _variables.size()) {
    return _variables.at(index);
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentParameterChanged(Timestamp ts, Engine& engine) {
  auto id = static_cast<size_t>(_state.value(ts));
  assert(id < _variables.size());
  notifyIntChanged(ts, engine, id);
}

void AllDifferent::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);

  for (auto& localValue : _localValues) {
    localValue.commitIf(ts);
  }

  for (SavedInt& savedInt : _counts) {
    savedInt.commitIf(ts);
  }
}
