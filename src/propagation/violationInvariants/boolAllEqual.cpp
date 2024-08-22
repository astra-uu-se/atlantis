#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"

#include <cassert>
#include <limits>

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
BoolAllEqual::BoolAllEqual(SolverBase& solver, VarId violationId,
                           std::vector<VarId>&& vars)
    : ViolationInvariant(solver, violationId),
      _vars(std::move(vars)),
      _numTrue(NULL_TIMESTAMP, 0) {
  _varNotified.reserve(_vars.size());
  for (size_t i = 0; i < _vars.size(); ++i) {
    _varNotified.emplace_back(NULL_TIMESTAMP, 0);
  }
}

void BoolAllEqual::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  registerDefinedVar(_violationId);
}

void BoolAllEqual::updateBounds(bool widenOnly) {
  _solver.updateBounds(_violationId, 0, static_cast<Int>(_vars.size()) / 2,
                       widenOnly);
}

void BoolAllEqual::recompute(Timestamp ts) {
  Int numTrue = 0;
  for (const auto& var : _vars) {
    numTrue += _solver.value(ts, var) == 0 ? 1 : 0;
  }

  _numTrue.setValue(ts, numTrue);

  assert(0 <= _numTrue.value(ts) && numTrue <= static_cast<Int>(_vars.size()));

  updateValue(ts, _violationId,
              std::min(numTrue, static_cast<Int>(_vars.size()) - numTrue));
}

void BoolAllEqual::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _vars.size());
  const bool newValue = _solver.value(ts, _vars[id]) == 0;
  const bool committedValue = _solver.committedValue(_vars[id]) == 0;
  assert(_varNotified[id].value(ts) == 0);
  _varNotified[id].setValue(ts, 1);
  if (newValue == committedValue) {
    return;
  }

  _numTrue.incValue(ts, newValue ? 1 : -1);

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

  _numTrue.commitIf(ts);

#ifndef NDEBUG

  Int numTrue = 0;
  for (const auto& var : _vars) {
    numTrue += _solver.value(ts, var) == 0 ? 1 : 0;
  }
  assert(_numTrue.committedValue() == numTrue);
#endif
}

}  // namespace atlantis::propagation
