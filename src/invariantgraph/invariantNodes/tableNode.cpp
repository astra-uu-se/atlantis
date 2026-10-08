#include "atlantis/invariantgraph/invariantNodes/tableNode.hpp"

#include <algorithm>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

#include "../implicitRanks.hpp"
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

TableNode::TableNode(InvariantGraph& graph,
                     std::vector<std::shared_ptr<VarNode>>&& outputs,
                     VarNode& input, std::vector<std::vector<Int>>&& table,
                     const size_t inputColumnIndex)
    : InvariantNode(graph, std::move(outputs), {input.ptr()}),
      _table(std::move(moveInputFirst(std::move(table), inputColumnIndex))) {
  assert(!_table.empty());
  assert(_table.front().size() == outputVarNodes().size() + 1);
}

TableNode::TableNode(InvariantGraph& graph,
                     std::vector<std::shared_ptr<VarNode>>&& outputs,
                     VarNode& input,
                     const std::vector<std::vector<bool>>& table,
                     const size_t inputColumnIndex)
    : TableNode(graph, std::move(outputs), input, boolToViol(table),
                inputColumnIndex) {}

void TableNode::init() {
  InvariantNode::init();
  assert(staticInputVarNodes().size() == 1);
  assert(std::ranges::all_of(staticInputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return vNode->isIntVar() ==
                                      staticInputVarNode(0).isIntVar();
                             }));
  assert(std::ranges::all_of(outputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return vNode->isIntVar() ==
                                      staticInputVarNode(0).isIntVar();
                             }));
  assert(staticInputVarNode(0).isIntVar() || _table.size() <= 2);
}

void TableNode::postConstraint() {
  InvariantNode::postConstraint();
  std::vector<ConstraintVarId> inputs(outputVarNodes().size() + 1,
                                      ConstraintVarId{NULL_NODE_ID});
  inputs.front() = staticInputVarNode(0).constraintVarId();
  for (size_t i = 0; i < outputVarNodes().size(); ++i) {
    inputs[i + 1] = outputVarNode(i).constraintVarId();
  }

  if (staticInputVarNode(0).isIntVar()) {
    return invariantGraph().constraintSolver().fzn_table_int(inputs, _table,
                                                             true);
  }
  return invariantGraph().constraintSolver().fzn_table_bool(
      inputs, violToBool(_table), true);
}

void TableNode::removeOutputVarNode(VarNode& outputVarNodeId) {
  for (Int i = static_cast<Int>(outputVarNodes().size()) - 1; i >= 0; --i) {
    if (&outputVarNode(i) == &outputVarNodeId) {
      removeColumn(i + 1);
    }
  }
  InvariantNode::removeOutputVarNode(outputVarNodeId);
  assert(_table.empty() || outputVarNodes().size() + 1 == numCols());
}

void TableNode::removeOutputAtIndex(const size_t index) {
  assert(_table.empty() || index + 1 < numCols());
  removeColumn(index + 1);
  InvariantNode::removeOutputAtIndex(index);
  assert(_table.empty() || outputVarNodes().size() + 1 == numCols());
}

size_t TableNode::numCols() const { return _table.front().size(); }

VarNode& TableNode::colVar(const size_t index) const {
  if (index == 0) {
    return staticInputVarNode(0);
  }
  return outputVarNode(index - 1);
}

void TableNode::removeRows() {
  std::vector<size_t> invalidRows;
  invalidRows.reserve(_table.size());
  for (size_t r = 0; r < _table.size(); ++r) {
    for (size_t c = 0; c < numCols(); ++c) {
      if (!colVar(c).constDomain()->contains(_table[r][c])) {
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
    if (&outputVarNode(index) != &staticInputVarNode(0)) {
      continue;
    }
    for (size_t r = 0; r < _table.size(); ++r) {
      invalidRow[r] = invalidRow[r] || _table[r][0] != _table[r][c];
    }
    removeOutputAtIndex(index);
    removeColumn(c);
  }
  for (Int i = 0; i < static_cast<Int>(outputVarNodes().size()); ++i) {
    for (Int j = static_cast<Int>(outputVarNodes().size() - 1); j > i; --j) {
      if (&outputVarNode(i) != &outputVarNode(j)) {
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
  if (staticInputVarNode(0).isFixed()) {
    _table.clear();
    assert(std::ranges::all_of(outputVarNodes(),
                               [&](const std::shared_ptr<VarNode>& vNode) {
                                 return vNode->isFixed();
                               }));
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  for (Int c = static_cast<Int>(numCols()) - 1; c >= 1; --c) {
    const size_t index = c - 1;
    if (!outputVarNode(index).isFixed()) {
      continue;
    }
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
  if (outputVarNodes().empty()) {
    if (!staticInputVarNodes().empty()) {
      staticInputVarNode(0).tightenDomainType();
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  if (_table.empty() || _table.front().empty()) {
    throw InconsistencyException("TableNode::updateState: Table is empty");
  }
}

bool TableNode::constrainsOutput(const VarNode& outputVarNodeId) const {
  for (size_t i = 0; i < outputVarNodes().size(); ++i) {
    if (&outputVarNode(i) != &outputVarNodeId) {
      continue;
    }
    std::vector<Int> vals(_table.size());
    for (size_t r = 0; r < _table.size(); ++r) {
      vals[r] = _table[r][i + 1];
    }
    const SortedUniqueVector sortedVals(std::move(vals));
    if (!outputVarNode(i).constDomain()->contains(sortedVals)) {
      return true;
    }
  }
  return false;
}

std::pair<size_t, size_t> TableNode::implicitRank() const {
  return {rank::IMPLICIT_RANK_TABLE,
          staticInputVarNodes().size() + outputVarNodes().size()};
}

bool TableNode::canBeMadeImplicit() const {
  return state() != InvariantNodeState::SUBSUMED &&
         staticInputVarNode(0).definingNodes().empty();
}

bool TableNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }
  std::vector<std::shared_ptr<VarNode>> vars;
  vars.reserve(_table.size());
  vars.emplace_back(staticInputVarNodes().front());
  for (const auto& vNode : outputVarNodes()) {
    vars.emplace_back(vNode);
  }
  invariantGraph().addImplicitConstraintNode(
      std::make_shared<TableImplicitNode>(invariantGraph(), std::move(vars),
                                          std::move(_table)));
  return true;
}

void TableNode::registerOutputVars(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  for (size_t i = 0; i < outputVarNodes().size(); ++i) {
    assert(std::ranges::none_of(outputVarNodes().begin(),
                                outputVarNodes().begin() + static_cast<Int>(i),
                                [&](const std::shared_ptr<VarNode>& vNode) {
                                  return vNode.get() == &outputVarNode(i);
                                }));

    assert(mapping.solverId(outputVarNode(i)) == propagation::NULL_ID);
    makeSolverVar(outputVarNode(i), solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void TableNode::registerNode(propagation::SolverBase& solver,
                             SolverMapping& mapping) const {
  const propagation::VarViewId inputVarId =
      mapping.solverId(staticInputVarNode(0));

  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(outputVarNodes().size());
  for (const auto& outVar : outputVarNodes()) {
    assert(mapping.solverId(outVar).isVar());
    outputVarIds.emplace_back(mapping.solverId(outVar));
  }

  if (staticInputVarNode(0).isIntVar()) {
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
