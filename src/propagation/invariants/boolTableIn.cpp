#include "atlantis/propagation/invariants/boolTableIn.hpp"

#include <cassert>
#include <ranges>

#include "./invariantHelper.hpp"

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

BoolTableIn::BoolTableIn(SolverBase& solver, std::vector<VarId>&& rowViolations,
                           std::vector<VarViewId>&& vars, const std::vector<std::vector<bool>>& table)
    : Invariant(solver),
      _varArray(std::move(vars)),
  _transposed(transpose(table)),
  _rowViolations(std::move(rowViolations))
  {}

BoolTableIn::BoolTableIn(SolverBase& solver, std::vector<VarViewId>&& rowViolations, std::vector<VarViewId>&& vars, const std::vector<std::vector<bool>>& table)
  : BoolTableIn(solver, toVarIds(std::move(rowViolations)), std::move(vars), table) {}

void BoolTableIn::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  for (const VarId rw : _rowViolations) {
    registerDefinedVar(rw);
  }
}

void BoolTableIn::updateBounds(const bool widenOnly) {
  for (const VarId rw : _rowViolations) {
    _solver.updateBounds(rw, 0, static_cast<Int>(_varArray.size()),
                       widenOnly);
  }
}

void BoolTableIn::recompute(const Timestamp ts) {
  // reduce violation for all active rows:
  for (size_t r = 0; r < _transposed.front().size(); ++r) {
    Int violation = 0;
    for (size_t c = 0; c < _varArray.size(); ++c) {
      violation += _transposed[c][r] == (_solver.value(ts, _varArray[c]) == 0) ? 0 : 1;
    }
    updateValue(ts, _rowViolations[r], violation);
  }
}

void BoolTableIn::notifyInputChanged(const Timestamp ts, const LocalId id) {
  assert(id < _varArray.size());
  const bool newValue = _solver.value(ts, _varArray[id]) == 0;
  const bool committedValue = _solver.committedValue(_varArray[id]) == 0;
  if (newValue == committedValue) {
    return;
  }
  for (size_t r = 0; r < _transposed[id].size(); ++r) {
    incValue(ts, _rowViolations[r], newValue == _transposed[id][r] ? -1 : 1);
  }
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

}  // namespace atlantis::propagation
