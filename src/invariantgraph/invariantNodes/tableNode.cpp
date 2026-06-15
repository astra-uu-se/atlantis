#include "atlantis/invariantgraph/invariantNodes/tableNode.hpp"

#include <algorithm>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/tableImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/countNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/boolTable.hpp"
#include "atlantis/propagation/invariants/table.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

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
                     const std::vector<std::vector<bool>>& table,
                     const size_t inputColumnIndex)
    : TableNode(graph, std::move(outputs), input, boolToViol(table),
                inputColumnIndex) {}

void TableNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(staticInputVarNodeIds().size() == 1);
  assert(std::ranges::all_of(staticInputVarNodeIds(), [&](const VarNodeId vId) {
    return varNodeConst(vId).isIntVar() ==
           staticInputVarNodeConst(0).isIntVar();
  }));
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return varNodeConst(vId).isIntVar() ==
           staticInputVarNodeConst(0).isIntVar();
  }));
  assert(staticInputVarNodeConst(0).isIntVar() || _table.size() <= 2);
}

void TableNode::postConstraint() {
  InvariantNode::postConstraint();
  std::vector<ConstraintVarId> inputs(outputVarNodeIds().size() + 1,
                                      ConstraintVarId{NULL_NODE_ID});
  inputs.front() = staticInputVarNodeConst(0).constraintVarId();
  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    inputs[i + 1] = outputVarNodeConst(i).constraintVarId();
  }

  if (staticInputVarNodeConst(0).isIntVar()) {
    return invariantGraph().constraintSolver().fzn_table_int(inputs, _table,
                                                             true);
  }
  return invariantGraph().constraintSolver().fzn_table_bool(
      inputs, violToBool(_table), true);
}

size_t TableNode::numCols() const { return _table.front().size(); }

VarNodeId TableNode::colVar(const size_t index) const {
  if (index == 0) {
    return staticInputVarNodeIds().front();
  }
  return outputVarNodeIds().at(index - 1);
}

void TableNode::removeRows() {
  std::vector<size_t> invalidRows;
  invalidRows.reserve(_table.size());
  for (size_t r = 0; r < _table.size(); ++r) {
    for (size_t c = 0; c < numCols(); ++c) {
      if (!varNodeConst(colVar(c)).constDomain()->contains(_table[r][c])) {
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
}

void TableNode::removeColumn(const size_t colIndex) {
  for (auto& row : _table) {
    assert(colIndex < row.size());
    row.erase(row.begin() + static_cast<Int>(colIndex));
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

void TableNode::removeColumns() {
  if (staticInputVarNodeConst(0).isFixed()) {
    _table.clear();
    assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
      return varNodeConst(vId).isFixed();
    }));
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  for (Int c = static_cast<Int>(numCols()) - 1; c >= 1; --c) {
    const size_t index = c - 1;
    if (!outputVarNodeConst(index).isFixed()) {
      continue;
    }
    removeColumn(c);
    removeOutputAtIndex(index);
  }
}

void TableNode::updateState() {
  removeColumns();
  if (state() == InvariantNodeState::SUBSUMED) {
    return;
  }
  removeDuplicateColumns();
  if (state() == InvariantNodeState::SUBSUMED) {
    return;
  }
  removeRows();
  if (state() == InvariantNodeState::SUBSUMED) {
    return;
  }
  if (outputVarNodeIds().empty()) {
    if (!staticInputVarNodeIds().empty()) {
      staticInputVarNode(0).tightenDomainType();
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  if (_table.empty() || _table.front().empty()) {
    throw InconsistencyException("TableNode::updateState: Table is empty");
  }
}

bool TableNode::canBeMadeImplicit() const {
  return state() != InvariantNodeState::SUBSUMED &&
         staticInputVarNodeConst(0).definingNodes().empty();
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
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
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

  if (staticInputVarNodeConst(0).isIntVar()) {
    solver.makeInvariant<propagation::Table>(
        solver, std::move(outputVarIds), inputVarId,
        std::vector<std::vector<Int>>{_table}, 0);
  } else {
    assert(_table.size() == 2);
    std::array<std::vector<Int>, 2> violTable;
    for (size_t row = 0; row < 2; ++row) {
      violTable[row] = _table[row];
    }
    solver.makeInvariant<propagation::BoolTable>(
        solver, std::move(outputVarIds), inputVarId, std::move(violTable), 0);
  }
}

std::string TableNode::dotLangIdentifier() const { return {"table"}; }

}  // namespace atlantis::invariantgraph
