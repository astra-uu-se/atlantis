#include "propagation/violationInvariants/boolAllEqual.hpp"

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
BoolAllEqual::BoolAllEqual(SolverBase& solver, VarId violationId,
                           std::vector<VarId>&& vars)
    : ViolationInvariant(solver, violationId),
      _vars(std::move(vars)),
      _committedValues(_vars.size(), 0),
      _numTrue(NULL_TIMESTAMP, 0) {
  _modifiedVars.reserve(_vars.size());
}

void BoolAllEqual::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  registerDefinedVar(_violationId);
}

void BoolAllEqual::updateBounds(bool widenOnly) {
  _solver.updateBounds(_violationId, 0, static_cast<Int>(_vars.size()) / 2, widenOnly);
}

void BoolAllEqual::close(Timestamp) {}

void BoolAllEqual::recompute(Timestamp ts) {
  _numTrue.setValue(ts, 0);

  for (size_t i = 0; i < _vars.size(); ++i) {
    _numTrue.incValue(ts, static_cast<Int>(_solver.value(ts, _vars[i]) == 0));
  }

  assert(0 <= _numTrue.value(ts) &&
         _numTrue.value(ts) <= static_cast<Int>(_vars.size()));

  updateValue(ts, _violationId,
              std::min(_numTrue.value(ts),
                       static_cast<Int>(_vars.size()) - _numTrue.value(ts)));
}

void BoolAllEqual::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _committedValues.size());
  const Int newValue = _solver.value(ts, _vars[id]);
  if ((newValue == 0) == (_committedValues[id] == 0)) {
    return;
  }

  _numTrue.incValue(ts, static_cast<Int>(newValue == 0) -
                            static_cast<Int>(_committedValues[id] == 0));

  assert(0 <= _numTrue.value(ts) &&
         _numTrue.value(ts) <= static_cast<Int>(_vars.size()));

  updateValue(ts, _violationId,
              std::min(_numTrue.value(ts),
                       static_cast<Int>(_vars.size()) - _numTrue.value(ts)));
}

VarId BoolAllEqual::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _vars.size()) {
    return _vars[index];
  }
  return NULL_ID;
}

void BoolAllEqual::notifyCurrentInputChanged(Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) < _vars.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void BoolAllEqual::commit(Timestamp ts) {
  Invariant::commit(ts);

  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _solver.committedValue(_vars[i]);
  }

  _numTrue.commitIf(ts);
}
}  // namespace atlantis::propagation