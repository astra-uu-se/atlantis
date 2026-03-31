#include "atlantis/invariantgraph/invariantNodes/tableNode.hpp"

#include <algorithm>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/tableImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/table.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

static std::vector<std::vector<Int>> toIntTable(
    std::vector<std::vector<bool>>&& boolTable) {
  std::vector<std::vector<Int>> intTable(boolTable.size());
  for (size_t r = 0; r < boolTable.size(); ++r) {
    intTable[r].resize(boolTable[r].size());
    for (size_t c = 0; c < boolTable[r].size(); ++c) {
      intTable[r][c] = boolTable[r][c] ? 0 : 1;
    }
  }
  return intTable;
}

static std::vector<std::vector<Int>>&& moveInputFirst(
    std::vector<std::vector<Int>>&& table, const size_t inputColumn) {
  assert(inputColumn < table.front().size());
  for (auto& r : table) {
    const Int inputVal = r[inputColumn];
    for (size_t c = inputColumn; c >= 1; --c) {
      r[c] = r[c - 1];
    }
    r[0] = inputVal;
  }
  return std::move(table);
}

TableNode::TableNode(InvariantGraph& graph, std::vector<VarNodeId>&& outputs,
                     const VarNodeId input,
                     std::vector<std::vector<Int>>&& table,
                     const size_t inputColumnIndex)
    : InvariantNode(graph, std::move(outputs), {input}),
      _table(std::move(moveInputFirst(std::move(table), inputColumnIndex))) {
  assert(!_table.empty());
  assert(_table.front().size() == outputVarNodeIds().size() + 1);
}

TableNode::TableNode(InvariantGraph& graph, std::vector<VarNodeId>&& outputs,
                     const VarNodeId input,
                     std::vector<std::vector<bool>>&& table,
                     const size_t inputColumnIndex)
    : TableNode(graph, std::move(outputs), input,
                toIntTable(std::move(table)), inputColumnIndex) {}

void TableNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(staticInputVarNodeIds().size() == 1);
  assert(std::ranges::all_of(staticInputVarNodeIds(), [&](const VarNodeId vId) {
    return invariantGraphConst().varNodeConst(vId).isIntVar() == invariantGraphConst().varNodeConst(staticInputVarNodeIds().front()).isIntVar();
  }));
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return invariantGraphConst().varNodeConst(vId).isIntVar() == invariantGraphConst().varNodeConst(staticInputVarNodeIds().front()).isIntVar();
  }));
}

size_t TableNode::numCols() const { return _table.front().size(); }

VarNodeId TableNode::colVar(const size_t index) const {
  if (index == 0) {
    return staticInputVarNodeIds().front();
  }
  return outputVarNodeIds().at(index - 1);
}

bool TableNode::removeRows() {
  std::vector<size_t> invalidRows;
  invalidRows.reserve(_table.size());
  for (size_t r = 0; r < _table.size(); ++r) {
    for (size_t c = 0; c < numCols(); ++c) {
      if (!invariantGraphConst()
               .varNodeConst(colVar(c))
               .constDomain()
               ->contains(_table[r][c])) {
        invalidRows.emplace_back(r);
        break;
      }
    }
  }
  for (Int index = static_cast<Int>(invalidRows.size()) - 1; index >= 0;
       --index) {
    const size_t invalidRow = invalidRows[index];
    std::swap(_table.back(), _table[invalidRow]);
    _table.pop_back();
  }
  return !invalidRows.empty();
}

void TableNode::removeColumn(const size_t index) {
  for (auto& row : _table) {
    assert(index < row.size());
    row.erase(row.begin() + index);
  }
}

void TableNode::removeDuplicateColumns() {
  std::vector<bool> invalidRow(_table.size(), false);
  for (Int c = static_cast<Int>(numCols()) - 1; c >= 1; --c) {
    const size_t index = c - 1;
    if (outputVarNodeIds().at(index) != staticInputVarNodeIds().front()) {
      continue;
    }
    for (size_t r = 0; r < _table.size(); ++r) {
      invalidRow[r] = invalidRow[r] || _table[r][0] != _table[r][c];
    }
    removeOutputAtIndex(index);
    removeColumn(c);
  }
  for (Int i = 0; i < static_cast<Int>(outputVarNodeIds().size()); ++i) {
    for (Int j = static_cast<Int>(outputVarNodeIds().size() - 1); j > i; --j) {
      if (outputVarNodeIds().at(i) != outputVarNodeIds().at(j)) {
        continue;
      }
      for (size_t r = 0; r < _table.size(); ++r) {
        invalidRow[r] = invalidRow[r] || _table[r][i + 1] != _table[r][j + 1];
      }
      removeOutputAtIndex(j);
      removeColumn(j + 1);
    }
  }
  for (size_t r1 = 0; r1 < _table.size(); ++r1) {
    if (invalidRow[r1]) {
      continue;
    }
    for (size_t r2 = r1 + 1; r2 < _table.size(); ++r2) {
      if (invalidRow[r2]) {
        continue;
      }
      bool sameRow = true;
      for (size_t c = 0; c < numCols(); ++c) {
        if (_table[r1][c] != _table[r2][c]) {
          sameRow = false;
          break;
        }
      }
      invalidRow[r2] = sameRow;
    }
  }
  for (Int r = static_cast<Int>(_table.size()) - 1; r >= 0; --r) {
    if (invalidRow[r]) {
      std::swap(_table.back(), _table[r]);
      _table.pop_back();
    }
  }
}

bool TableNode::propagate() {
  bool prunedVals = false;
  for (Int c = static_cast<Int>(numCols()) - 1; c >= 0; --c) {
    std::vector<Int> values(_table.size());
    values.reserve(_table.size());
    for (size_t r = 0; r < _table.size(); ++r) {
      values[r] = _table[r][c];
    }
    SortedUniqueVector sortedValues(std::move(values));
    const size_t prevDomSize =
        invariantGraphConst().varNodeConst(colVar(c)).constDomain()->size();
    if (c != 0 && (*sortedValues).size() == 1) {
      invariantGraph().varNode(colVar(c)).domain()->fix(
          (*sortedValues).front());
      removeColumn(c);
      removeOutputAtIndex(c - 1);
      prunedVals |= prevDomSize > 1;
    } else {
      invariantGraph().varNode(colVar(c)).domain()->removeAllValuesExcept(
          sortedValues);
      prunedVals |=
          prevDomSize !=
          invariantGraphConst().varNodeConst(colVar(c)).constDomain()->size();
    }
  }
  return prunedVals;
}

void TableNode::updateState() {
  removeDuplicateColumns();
  while (true) {
    const bool didPruneVals = propagate();
    const bool didRemoveRows = removeRows();
    if (!didPruneVals && !didRemoveRows) {
      break;
    }
  }
  if (_table.empty() || _table.front().empty()) {
    throw InconsistencyException("TableNode::updateState: Table is empty");
  }
  if (_table.size() == 1 || _table.front().size() == 1) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool TableNode::canBeMadeImplicit() const {
  return state() != InvariantNodeState::SUBSUMED &&
         invariantGraphConst()
             .varNodeConst(staticInputVarNodeIds().front())
             .definingNodes()
             .empty();
}

bool TableNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }
  std::vector<VarNodeId> vars;
  vars.reserve(_table.size());
  vars.emplace_back(staticInputVarNodeIds().front());
  for (const auto vId : outputVarNodeIds()) {
    vars.emplace_back(vId);
  }
  invariantGraph().addImplicitConstraintNode(
      std::make_shared<TableImplicitNode>(invariantGraph(), std::move(vars),
                                          std::move(_table)));
  return true;
}

void TableNode::registerOutputVars(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    assert(std::ranges::none_of(
        outputVarNodeIds().begin(),
        outputVarNodeIds().begin() + static_cast<Int>(i),
        [&](const VarNodeId vId) { return vId == outputVarNodeIds().at(i); }));

    assert(mapping.solverId(outputVarNodeIds().at(i)) == propagation::NULL_ID);
    makeSolverVar(outputVarNodeIds().at(i), solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void TableNode::registerNode(propagation::SolverBase& solver,
                             SolverMapping& mapping) const {
  const propagation::VarViewId inputVarId =
      mapping.solverId(staticInputVarNodeIds().front());

  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(outputVarNodeIds().size());
  for (const VarNodeId outVarId : outputVarNodeIds()) {
    assert(mapping.solverId(outVarId).isVar());
    outputVarIds.emplace_back(mapping.solverId(outVarId));
  }

  solver.makeInvariant<propagation::Table>(
      solver, std::move(outputVarIds), inputVarId,
      std::vector<std::vector<Int>>(_table), 0);
}

std::string TableNode::dotLangIdentifier() const { return {"table"}; }

}  // namespace atlantis::invariantgraph
