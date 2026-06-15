#include "atlantis/propagation/invariants/boolTable.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <ranges>

#include "./invariantHelper.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

std::array<std::vector<Int>, 2>&& translateBoolTable(
    std::array<std::vector<Int>, 2>&& table, const size_t inputColumn) {
  assert(table.size() == 2);
  assert(inputColumn < table.front().size());
  assert((table.front().at(inputColumn) == 0) !=
         (table.back().at(inputColumn) == 0));
  if (table.front()[inputColumn] == 0) {
    std::swap(table.front(), table.back());
  }
  for (size_t r = 0; r < table.size(); ++r) {
    for (size_t c = inputColumn; c + 1 < table[r].size(); ++c) {
      table[r][c] = table[r][c + 1];
    }
    table[r].resize(table[r].size() - 1);
  }
  return std::move(table);
}

BoolTable::BoolTable(SolverBase& solver, std::vector<VarId>&& outputVars,
                     const VarViewId inputVar,
                     std::array<std::vector<Int>, 2>&& table,
                     const size_t inputColumn)
    : Invariant(solver),
      _inputVar(inputVar),
      _outputVars(std::move(outputVars)),
      _table(std::move(translateBoolTable(std::move(table), inputColumn))) {
  assert(std::ranges::all_of(_table, [&](const std::vector<Int>& row) {
    return row.size() == _outputVars.size();
  }));
}

BoolTable::BoolTable(SolverBase& solver, std::vector<VarViewId>&& outputVars,
                     const VarViewId inputVar,
                     std::array<std::vector<Int>, 2>&& table,
                     const size_t inputColumn)
    : BoolTable(solver, toVarIds(std::move(outputVars)), inputVar,
                std::move(table), inputColumn) {}

void BoolTable::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _inputVar, 0, false);
  for (const VarId outputVar : _outputVars) {
    registerDefinedVar(outputVar);
  }
}

void BoolTable::updateBounds(const bool widenOnly) {
  std::vector<std::array<Int, 2>> bounds(
      _outputVars.size(),
      {std::numeric_limits<Int>::max(), std::numeric_limits<Int>::min()});
  const Int rowLb =
      _solver.lowerBound(_inputVar) != 0 || _solver.upperBound(_inputVar) != 0
          ? 0
          : 1;
  const Int rowUb =
      _solver.lowerBound(_inputVar) <= 0 && 0 <= _solver.upperBound(_inputVar)
          ? 1
          : 0;

  for (Int r = rowLb; r <= rowUb; ++r) {
    for (size_t c = 0; c < _table[r].size(); ++c) {
      bounds[c][0] = std::min(bounds[c][0], _table[r][c]);
      bounds[c][1] = std::max(bounds[c][1], _table[r][c]);
    }
  }

  for (size_t c = 0; c < bounds.size(); ++c) {
    _solver.updateBounds(_outputVars[c], bounds[c][0], bounds[c][1], widenOnly);
  }
}

void BoolTable::recompute(const Timestamp ts, const bool forceRecompute) {
  const bool val = _solver.value(ts, _inputVar) == 0;
  if (!forceRecompute && val == (_solver.committedValue(_inputVar) == 0)) {
    return;
  }
  const size_t row = val ? 1 : 0;
  for (size_t c = 0; c < _outputVars.size(); ++c) {
    updateValue(ts, _outputVars[c], _table[row][c]);
  }
}

void BoolTable::recompute(const Timestamp ts) { recompute(ts, true); }

void BoolTable::notifyInputChanged(const Timestamp ts, const LocalId) {
  recompute(ts, false);
}

VarViewId BoolTable::nextInput(const Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index == 0) {
    return _inputVar;
  }
  return NULL_ID;
}

void BoolTable::notifyCurrentInputChanged(const Timestamp ts) {
  recompute(ts, false);
}

}  // namespace atlantis::propagation
