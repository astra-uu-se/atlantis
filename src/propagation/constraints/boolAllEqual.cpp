#include "propagation/constraints/boolAllEqual.hpp"

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
BoolAllEqual::BoolAllEqual(Engine& engine, VarId violationId,
                           std::vector<VarId> variables)
    : Constraint(engine, violationId),
      _variables(std::move(variables)),
      _committedValues(_variables.size(), 0),
      _numTrue(NULL_TIMESTAMP, 0) {
  _modifiedVars.reserve(_variables.size());
}

void BoolAllEqual::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _variables.size(); ++i) {
    _engine.registerInvariantInput(_id, _variables[i], i);
  }
  registerDefinedVariable(_violationId);
}

void BoolAllEqual::updateBounds(bool widenOnly) {
  _engine.updateBounds(_violationId, 0, _variables.size() / 2, widenOnly);
}

void BoolAllEqual::close(Timestamp) {}

void BoolAllEqual::recompute(Timestamp ts) {
  _numTrue.setValue(ts, 0);

  for (size_t i = 0; i < _variables.size(); ++i) {
    _numTrue.incValue(ts,
                      static_cast<Int>(_engine.value(ts, _variables[i]) == 0));
  }

  assert(0 <= _numTrue.value(ts) &&
         _numTrue.value(ts) <= static_cast<Int>(_variables.size()));

  updateValue(ts, _violationId,
              std::min(_numTrue.value(ts), static_cast<Int>(_variables.size()) -
                                               _numTrue.value(ts)));
}

void BoolAllEqual::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _committedValues.size());
  const Int newValue = _engine.value(ts, _variables[id]);
  if ((newValue == 0) == (_committedValues[id] == 0)) {
    return;
  }

  _numTrue.incValue(ts, static_cast<Int>(newValue == 0) -
                            static_cast<Int>(_committedValues[id] == 0));

  assert(0 <= _numTrue.value(ts) &&
         _numTrue.value(ts) <= static_cast<Int>(_variables.size()));

  updateValue(ts, _violationId,
              std::min(_numTrue.value(ts), static_cast<Int>(_variables.size()) -
                                               _numTrue.value(ts)));
}

VarId BoolAllEqual::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _variables.size()) {
    return _variables[index];
  }
  return NULL_ID;
}

void BoolAllEqual::notifyCurrentInputChanged(Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) < _variables.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void BoolAllEqual::commit(Timestamp ts) {
  Invariant::commit(ts);

  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _engine.committedValue(_variables[i]);
  }

  _numTrue.commitIf(ts);
}
}  // namespace atlantis::propagation