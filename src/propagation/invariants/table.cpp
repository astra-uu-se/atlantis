#include "atlantis/propagation/invariants/table.hpp"

#include <cassert>
#include <ranges>

#include "./invariantHelper.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

std::vector<std::unordered_map<Int, std::vector<size_t>>> generateValToRows(const std::vector<std::vector<Int>>& table) {
  std::vector<std::unordered_map<Int, std::vector<size_t>>> valToRows(table.front().size(), std::unordered_map<Int, std::vector<size_t>>(table.size()));
  for (size_t r = 0; r < table.size(); ++r) {
    for (size_t c = 0; c < table[r].size(); ++c) {
      const Int val = table[r][c];
      const auto& iter = valToRows[c].find(val);
      if (iter == valToRows[r].end()) {
        valToRows[c].emplace(val, std::vector<size_t>{r});
      } else {
        iter->second.emplace_back(r);
      }
    }
  }
  return valToRows;
}

Table::Table(SolverBase& solver, std::vector<VarId>&& rowViolations,
                           std::vector<VarViewId>&& vars, const std::vector<std::vector<Int>>& table)
    : Invariant(solver),
      _varArray(std::move(vars)),
  _valToRows(generateValToRows(table)),
  _rowViolations(std::move(rowViolations))
  {}

Table::Table(SolverBase& solver, std::vector<VarViewId>&& rowViolations, std::vector<VarViewId>&& vars, const std::vector<std::vector<Int>>& table)
  : Table(solver, toVarIds(std::move(rowViolations)), std::move(vars), table) {}

void Table::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  for (const VarId rw : _rowViolations) {
    registerDefinedVar(rw);
  }
}

void Table::updateBounds(const bool widenOnly) {
  for (const VarId rw : _rowViolations) {
    _solver.updateBounds(rw, 0, static_cast<Int>(_varArray.size()),
                       widenOnly);

  }
}

void Table::close(const Timestamp) {
  // reduce the size of _valToVars:
  std::vector<Int> valsToRemove;
  for (size_t c = 0; c < _varArray.size(); ++c) {
    valsToRemove.clear();
    valsToRemove.reserve(_valToRows[c].size());
    const Int lb = _solver.lowerBound(_varArray[c]);
    const Int ub = _solver.upperBound(_varArray[c]);
    for (const Int val: std::views::keys(_valToRows[c])) {
      if (val < lb || ub < val) {
        valsToRemove.emplace_back(val);
      }
    }
    for (const Int v : valsToRemove) {
      _valToRows[c].erase(v);
    }
  }
}

void Table::recompute(const Timestamp ts) {
  std::vector<Int> violations(_rowViolations.size(), static_cast<Int>(_varArray.size()));

  // reduce violation for all active rows:
  for (size_t c = 0; c < _varArray.size(); ++c) {
    const Int val = _solver.value(ts, _varArray[c]);
    const auto& iter = _valToRows[c].find(val);
    if (iter != _valToRows[c].end()) {
      for (const size_t r : iter->second) {
        --violations[r];
      }
    }
  }
  // use min logic to keep track of min row:
  for (size_t r = 0; r < violations.size(); ++r) {
    updateValue(ts, _rowViolations[r], violations[r]);
  }
}

void Table::notifyInputChanged(const Timestamp ts, const LocalId id) {
  assert(id < _varArray.size());
  const Int newValue = _solver.value(ts, _varArray[id]);
  const Int committedValue = _solver.committedValue(_varArray[id]);
  if (newValue == committedValue) {
    return;
  }
  const auto& committedIter = _valToRows[id].find(committedValue);
  if (committedIter != _valToRows[id].end()) {
    for (const size_t committedRow : committedIter->second) {
      incValue(ts, _rowViolations[committedRow], 1);
    }
  }
  const auto& newIter = _valToRows[id].find(newValue);
  if (newIter != _valToRows[id].end()) {
    for (const size_t newRow : newIter->second) {
      incValue(ts, _rowViolations[newRow], -1);
    }
  }
}

VarViewId Table::nextInput(const Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _varArray.size()) {
    return _varArray[index];
  }
  return NULL_ID;
}

void Table::notifyCurrentInputChanged(const Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) < _varArray.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

}  // namespace atlantis::propagation
