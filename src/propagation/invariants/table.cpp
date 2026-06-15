#include "atlantis/propagation/invariants/table.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <ranges>

#include "./invariantHelper.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

std::unordered_map<Int, size_t> generateTrueRows(
    const std::vector<std::vector<Int>>& table, const size_t inputColumn) {
  std::unordered_map<Int, size_t> valToRows(
      std::unordered_map<Int, size_t>(table.size()));
  for (size_t r = 0; r < table.size(); ++r) {
    assert(table[r].size() > inputColumn);
    const Int val = table[r][inputColumn];
    assert(!valToRows.contains(val));
    valToRows.emplace(val, r);
  }
  return valToRows;
}

std::vector<std::vector<Int>>&& removeInputColumn(
    std::vector<std::vector<Int>>&& table, const size_t inputColumn) {
  assert(inputColumn < table.front().size());
  for (size_t r = 0; r < table.size(); ++r) {
    for (size_t c = inputColumn; c + 1 < table[r].size(); ++c) {
      table[r][c] = table[r][c + 1];
    }
    table[r].resize(table[r].size() - 1);
  }
  return std::move(table);
}

Table::Table(SolverBase& solver, std::vector<VarId>&& outputVars,
             const VarViewId inputVar, std::vector<std::vector<Int>>&& table,
             size_t inputColumn)
    : Invariant(solver),
      _inputVar(inputVar),
      _outputVars(std::move(outputVars)),
      _valToRow(
          generateTrueRows(table, inputColumn)),  // must come before _table
      _table(std::move(removeInputColumn(std::move(table), inputColumn))) {
  assert(!_table.empty());
  assert(std::ranges::all_of(_table, [&](const std::vector<Int>& row) {
    return row.size() == _outputVars.size();
  }));
  assert(_valToRow.size() == _table.size());
}

Table::Table(SolverBase& solver, std::vector<VarViewId>&& outputVars,
             const VarViewId inputVar, std::vector<std::vector<Int>>&& table,
             size_t inputColumn)
    : Table(solver, toVarIds(std::move(outputVars)), inputVar, std::move(table),
            inputColumn) {}

void Table::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _inputVar, 0, false);
  for (const VarId outputVar : _outputVars) {
    registerDefinedVar(outputVar);
  }
}

void Table::updateBounds(const bool widenOnly) {
  std::vector<std::array<Int, 2>> bounds(
      _outputVars.size(),
      {std::numeric_limits<Int>::max(), std::numeric_limits<Int>::min()});

  const Int lb = _solver.lowerBound(_inputVar);
  const Int ub = _solver.upperBound(_inputVar);

  for (const size_t r : std::ranges::views::values(_valToRow)) {
    if (static_cast<Int>(r) < lb || ub < static_cast<Int>(r)) {
      continue;
    }
    for (size_t c = 0; c < _table[r].size(); ++c) {
      bounds[c][0] = std::min(bounds[c][0], _table[r][c]);
      bounds[c][1] = std::max(bounds[c][1], _table[r][c]);
    }
  }

  for (size_t c = 0; c < bounds.size(); ++c) {
    _solver.updateBounds(_outputVars[c], bounds[c][0], bounds[c][1], widenOnly);
  }
}

void Table::close(const Timestamp) {
  // reduce the size of _valToVars:
  std::vector<Int> valsToRemove;
  valsToRemove.clear();
  valsToRemove.reserve(_valToRow.size());
  const Int lb = _solver.lowerBound(_inputVar);
  const Int ub = _solver.upperBound(_inputVar);
  for (const Int val : std::views::keys(_valToRow)) {
    if (val < lb || ub < val) {
      valsToRemove.emplace_back(val);
    }
  }
  for (const Int v : valsToRemove) {
    _valToRow.erase(v);
  }
}

void Table::recompute(const Timestamp ts, bool forceRecompute) {
  const Int val = _solver.value(ts, _inputVar);
  if (!forceRecompute && val == _solver.committedValue(_inputVar)) {
    return;
  }
  const auto& iter = _valToRow.find(val);
  const size_t row = iter != _valToRow.end() ? iter->second : size_t{0};
  for (size_t c = 0; c < _outputVars.size(); ++c) {
    updateValue(ts, _outputVars[c], _table[row][c]);
  }
}

void Table::recompute(const Timestamp ts) { recompute(ts, true); }

void Table::notifyInputChanged(const Timestamp ts, const LocalId) {
  recompute(ts, false);
}

VarViewId Table::nextInput(const Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index == 0) {
    return _inputVar;
  }
  return NULL_ID;
}

void Table::notifyCurrentInputChanged(const Timestamp ts) {
  recompute(ts, false);
}

}  // namespace atlantis::propagation
