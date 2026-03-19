#include "atlantis/propagation/violationInvariants/tableIn.hpp"

#include <cassert>
#include <ranges>

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

TableIn::TableIn(SolverBase& solver, VarId violationId,
                           std::vector<VarViewId>&& vars, const std::vector<std::vector<Int>>& table)
    : ViolationInvariant(solver, violationId),
      _varArray(std::move(vars)),
  _valToRows(generateValToRows(table)),
  _rowViolations(table.size(), {NULL_TIMESTAMP, -1, -1}),
  _violationCounts(_varArray.size() + 1, {NULL_TIMESTAMP, -1, -1})
  {}

TableIn::TableIn(SolverBase& solver, VarViewId violationId, std::vector<VarViewId>&& vars, const std::vector<std::vector<Int>>& table)
  : TableIn(solver, static_cast<VarId>(violationId), std::move(vars), table) {}

void TableIn::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  registerDefinedVar(_violationId);
}

void TableIn::updateBounds(const bool widenOnly) {
  _solver.updateBounds(_violationId, 0, static_cast<Int>(_varArray.size()),
                       widenOnly);
}

void TableIn::close(const Timestamp) {
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

void TableIn::recompute(const Timestamp ts) {
  for (auto& rw : _rowViolations) {
    rw.setValue(ts, static_cast<Int>(_varArray.size()));
  }

  // reduce violation for all active rows:
  for (size_t c = 0; c < _varArray.size(); ++c) {
    const Int val = _solver.value(ts, _varArray[c]);
    const auto& iter = _valToRows[c].find(val);
    if (iter != _valToRows[c].end()) {
      for (const size_t r : iter->second) {
        _rowViolations[r].incValue(ts, -1);
      }
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

void TableIn::notifyInputChanged(const Timestamp ts, const LocalId id) {
  assert(id < _varArray.size());
  const Int newValue = _solver.value(ts, _varArray[id]);
  const Int committedValue = _solver.committedValue(_varArray[id]);
  if (newValue == committedValue) {
    return;
  }
  assert(0 <= _solver.value(ts, _violationId));
  assert(_solver.value(ts, _violationId) <= static_cast<Int>(_varArray.size()));
  assert(_violationCounts.at(_solver.value(ts, _violationId)).value(ts) > 0);
  assert(std::all_of(_violationCounts.begin(), _violationCounts.begin() + _solver.value(ts, _violationId), [&](const CommittableInt& count) { return count.value(ts) == 0; }));
  const auto& committedIter = _valToRows[id].find(committedValue);
  // Increase row violations of committed rows.
  // Decrease the counts of committed rows.
  if (committedIter != _valToRows[id].end()) {
    for (const size_t committedRow : committedIter->second) {
      assert(_rowViolations[committedRow].value(ts) < static_cast<Int>(_varArray.size()));
      assert(_violationCounts.at(_rowViolations.at(committedRow).value(ts)).value(ts) > 0);
      assert(_violationCounts.at(_rowViolations.at(committedRow).value(ts) + 1).value(ts) < static_cast<Int>(_rowViolations.size()));
      _violationCounts[_rowViolations[committedRow].value(ts)].incValue(ts, -1);
      _violationCounts[_rowViolations[committedRow].incValue(ts, 1)].incValue(ts, 1);
    }
  }
  // Decrease row violations of committed rows.
  // Decrease the counts of committed rows.
  const auto& newIter = _valToRows[id].find(newValue);
  if (newIter != _valToRows[id].end()) {
    for (const size_t newRow : newIter->second) {
      assert(_rowViolations[newRow].value(ts) > 0);
      assert(_violationCounts.at(_rowViolations.at(newRow).value(ts)).value(ts) > 0);
      assert(_violationCounts.at(_rowViolations.at(newRow).value(ts) - 1).value(ts) < static_cast<Int>(_rowViolations.size()));
      _violationCounts[_rowViolations[newRow].value(ts)].incValue(ts, -1);
      _violationCounts[_rowViolations[newRow].incValue(ts, -1)].incValue(ts, 1);
    }
  }
  const Int violation = _solver.value(ts, _violationId);
  if (violation > 0 && _violationCounts[violation - 1].value(ts) > 0) {
    updateValue(ts, _violationId, violation - 1);
  } else if (_violationCounts[violation].value(ts) == 0) {
    assert(violation < static_cast<Int>(_varArray.size()));
    assert(_violationCounts.at(violation + 1).value(ts) > 0);
    updateValue(ts, _violationId, violation + 1);
  }
  assert(_violationCounts.at(_solver.value(ts, _violationId)).value(ts) > 0);
  assert(std::all_of(_violationCounts.begin(), _violationCounts.begin() + _solver.value(ts, _violationId), [&](const CommittableInt& count) { return count.value(ts) == 0; }));
}

VarViewId TableIn::nextInput(const Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _varArray.size()) {
    return _varArray[index];
  }
  return NULL_ID;
}

void TableIn::notifyCurrentInputChanged(const Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) < _varArray.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void TableIn::commit(const Timestamp timestamp) {
  ViolationInvariant::commit(timestamp);
  for (auto& rw : _rowViolations) {
    rw.commitIf(timestamp);
  }
  for (auto& vc : _violationCounts) {
    vc.commitIf(timestamp);
  }
}
}  // namespace atlantis::propagation
