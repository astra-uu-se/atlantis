#include "atlantis/propagation/violationInvariants/boolTableIn.hpp"

#include <cassert>
#include <ranges>

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

std::vector<std::vector<bool>> transpose(const std::vector<std::vector<bool>>& table) {
  std::vector<std::vector<bool>> t(table.front().size(), std::vector<bool>(table.size()));
  for (size_t r = 0; r < table.size(); ++r) {
    for (size_t c = 0; c < table[r].size(); ++c) {
      t[c][r] = table[r][c];
    }
  }
  return t;
}

BoolTableIn::BoolTableIn(SolverBase& solver, VarId violationId,
                           std::vector<VarViewId>&& vars, const std::vector<std::vector<bool>>& table)
    : ViolationInvariant(solver, violationId),
      _varArray(std::move(vars)),
  _transposed(transpose(table)),
  _rowViolations(_transposed.front().size(), {NULL_TIMESTAMP, -1, -1}),
  _violationCounts(_varArray.size() + 1, {NULL_TIMESTAMP, -1, -1})
  {}

BoolTableIn::BoolTableIn(SolverBase& solver, VarViewId violationId, std::vector<VarViewId>&& vars, const std::vector<std::vector<bool>>& table)
  : BoolTableIn(solver, static_cast<VarId>(violationId), std::move(vars), table) {}

void BoolTableIn::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  registerDefinedVar(_violationId);
}

void BoolTableIn::updateBounds(const bool widenOnly) {
  _solver.updateBounds(_violationId, 0, static_cast<Int>(_varArray.size()),
                       widenOnly);
}

void BoolTableIn::recompute(const Timestamp ts) {
  for (auto& rw : _rowViolations) {
    rw.setValue(ts, 0);
  }

  // reduce violation for all active rows:
  for (size_t c = 0; c < _varArray.size(); ++c) {
    const bool val = _solver.value(ts, _varArray[c]) == 0;
    for (size_t r = 0; r < _transposed[c].size(); ++r) {
      if (val != _transposed[c][r]) {
        _rowViolations[r].incValue(ts, 1);
      };
    }
  }
  for (auto& vc : _violationCounts) {
    vc.setValue(ts, 0);
  }
  Int violation = static_cast<Int>(_varArray.size());
  for (auto& rw : _rowViolations) {
    assert(0 <= rw.value(ts) && rw.value(ts) <= static_cast<Int>(_varArray.size()));
    _violationCounts[rw.value(ts)].incValue(ts, 1);
    violation = std::min(violation, rw.value(ts));
  }
  updateValue(ts, _violationId, violation);
}

void BoolTableIn::notifyInputChanged(const Timestamp ts, const LocalId id) {
  assert(id < _varArray.size());
  const bool newValue = _solver.value(ts, _varArray[id]) == 0;
  const bool committedValue = _solver.committedValue(_varArray[id]) == 0;
  if (newValue == committedValue) {
    return;
  }
  if (newValue == committedValue) {
    return;
  }
  assert(0 <= _solver.value(ts, _violationId));
  assert(_solver.value(ts, _violationId) <= static_cast<Int>(_varArray.size()));
  assert(_violationCounts.at(_solver.value(ts, _violationId)).value(ts) > 0);
  assert(std::all_of(_violationCounts.begin(), _violationCounts.begin() + _solver.value(ts, _violationId), [&](const CommittableInt& count) { return count.value(ts) == 0; }));
  Int minViolation = static_cast<Int>(_varArray.size());
  for (size_t r = 0; r < _transposed[id].size(); ++r) {
    _violationCounts[_rowViolations[r].value(ts)].incValue(ts, -1);
    const Int rowViol = _rowViolations[r].incValue(ts, newValue == _transposed[id][r] ? -1 : 1);
    _violationCounts[rowViol].incValue(ts, 1);
    minViolation = std::min(minViolation, rowViol);
  }
  assert(0 <= minViolation);
  assert(minViolation <= static_cast<Int>(_varArray.size()));
  assert(_violationCounts.at(minViolation).value(ts) > 0);
  updateValue(ts, _violationId, minViolation);
  assert(std::all_of(_violationCounts.begin(), _violationCounts.begin() + minViolation, [&](const CommittableInt& count) { return count.value(ts) == 0; }));
}

VarViewId BoolTableIn::nextInput(const Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _varArray.size()) {
    return _varArray[index];
  }
  return NULL_ID;
}

void BoolTableIn::notifyCurrentInputChanged(const Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) < _varArray.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void BoolTableIn::commit(const Timestamp timestamp) {
  ViolationInvariant::commit(timestamp);
  for (auto& rw : _rowViolations) {
    rw.commitIf(timestamp);
  }
  for (auto& vc : _violationCounts) {
    vc.commitIf(timestamp);
  }
}
}  // namespace atlantis::propagation
