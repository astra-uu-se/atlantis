#include "atlantis/invariantgraph/violationInvariantNodes/tableInNode.hpp"

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
#include "atlantis/invariantgraph/invariantNodes/tableNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolTableIn.hpp"
#include "atlantis/propagation/violationInvariants/tableIn.hpp"
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

TableInNode::TableInNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                         std::vector<std::vector<Int>>&& table,
                         const VarNodeId reified, const bool isBoolTable)
    : ViolationInvariantNode(graph, std::move(vars), reified),
      _table(std::move(table)),
      _isBoolTable(isBoolTable) {
  assert(_table.empty() ||
         _table.front().size() == staticInputVarNodeIds().size());
}

TableInNode::TableInNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                         std::vector<std::vector<Int>>&& table,
                         const bool shouldHold, const bool isBoolTable)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _table(std::move(table)),
      _isBoolTable(isBoolTable) {
  assert(_table.empty() ||
         _table.front().size() == staticInputVarNodeIds().size());
}

TableInNode::TableInNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                         std::vector<std::vector<bool>>&& table,
                         VarNodeId reified)
    : TableInNode(graph, std::move(vars), toIntTable(std::move(table)), reified,
                  true) {}

TableInNode::TableInNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                         std::vector<std::vector<bool>>&& table,
                         const bool shouldHold)
    : TableInNode(graph, std::move(vars), toIntTable(std::move(table)),
                  shouldHold, true) {}

void TableInNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(!staticInputVarNodeIds().empty());
  assert(std::ranges::all_of(staticInputVarNodeIds(), [&](const VarNodeId vId) {
    return invariantGraphConst().varNodeConst(vId).isIntVar() != _isBoolTable;
  }));
}

void TableInNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    if (_isBoolTable) {
      return constraintSolver().fzn_table_bool_reif(
          toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
          _table, reifiedVarNodeConst().constraintVarId());
    }
    return constraintSolver().fzn_table_int_reif(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        _table, reifiedVarNodeConst().constraintVarId());
  }
  if (_isBoolTable) {
    return constraintSolver().fzn_table_bool(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        violToBool(_table), shouldHold());
  }
  return constraintSolver().fzn_table_int(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      _table, shouldHold());
}

size_t TableInNode::numCols() const { return _table.front().size(); }

void TableInNode::removeInvalidRows() {
  std::vector<size_t> invalidRows;
  invalidRows.reserve(_table.size());
  for (size_t r = 0; r < _table.size(); ++r) {
    for (size_t c = 0; c < numCols(); ++c) {
      if (!staticInputVarNodeConst(c).constDomain()->contains(_table[r][c])) {
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
  if (_table.empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void TableInNode::removeColumn(const size_t colIndex) {
  for (auto& row : _table) {
    assert(colIndex < row.size());
    row.erase(row.begin() + static_cast<Int>(colIndex));
  }
}

void TableInNode::removeInvalidColumns() {
  for (Int i = static_cast<Int>(staticInputVarNodeIds().size()) - 1; i >= 0;
       --i) {
    if (!staticInputVarNodeConst(i).isFixed()) {
      continue;
    }
    const Int val = staticInputVarNodeConst(i).lowerBound();
    for (Int r = static_cast<Int>(_table.size()) - 1; r >= 0; --r) {
      if (_table[r][i] != val) {
        std::swap(_table[r], _table.back());
        _table.pop_back();
      }
    }
    removeColumn(i);
    removeStaticInputAtIndex(i);
  }
  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void TableInNode::removeDuplicateColumns() {
  std::vector<bool> invalidRow(_table.size(), false);
  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    for (Int j = static_cast<Int>(staticInputVarNodeIds().size() - 1); j > i;
         --j) {
      if (staticInputVarNodeIds().at(i) != staticInputVarNodeIds().at(j)) {
        continue;
      }
      for (size_t r = 0; r < _table.size(); ++r) {
        invalidRow[r] = invalidRow[r] || _table[r][i] != _table[r][j];
      }
      removeStaticInputAtIndex(j);
      removeColumn(j);
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

void TableInNode::updateState() {
  ViolationInvariantNode::updateState();
  if (isReified() || !shouldHold()) {
    return;
  }
  removeDuplicateColumns();
  removeInvalidRows();
  removeInvalidColumns();

  if (_table.size() <= 1 || _table.front().empty() ||
      staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

Int TableInNode::firstInputColIndex() const {
  if (isReified() || !shouldHold() || state() != InvariantNodeState::SUBSUMED) {
    return -1;
  }
  std::vector<bool> inputIsDefined(staticInputVarNodeIds().size(), false);
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    inputIsDefined[i] = !staticInputVarNodeConst(i).definingNodes().empty();
  }
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    if (staticInputVarNodeConst(i).constDomain()->size() != _table.size()) {
      continue;
    }
    bool replaceable = true;
    for (size_t c = 0; c < staticInputVarNodeIds().size(); ++c) {
      if (c == i) {
        continue;
      }
      if (inputIsDefined[i]) {
        replaceable = false;
        break;
      }
    }
    if (replaceable) {
      return static_cast<Int>(i);
    }
  }
  return -1;
}

bool TableInNode::canBeReplaced() const { return firstInputColIndex() >= 0; }

bool TableInNode::replace() {
  const Int inputColIndex = firstInputColIndex();
  if (inputColIndex < 0) {
    return false;
  }
  std::vector<VarNodeId> outputs;
  outputs.reserve(staticInputVarNodeIds().size() - 1);
  for (size_t c = 0; c < staticInputVarNodeIds().size(); ++c) {
    if (static_cast<Int>(c) != inputColIndex) {
      outputs.emplace_back(staticInputVarNodeIds().at(c));
    }
  }
  invariantGraph().addInvariantNode(
      std::make_shared<TableNode>(invariantGraph(), std::move(outputs),
                                  staticInputVarNodeIds().at(inputColIndex),
                                  std::move(_table), inputColIndex));
  return true;
}

bool TableInNode::canBeMadeImplicit() const {
  return !isReified() && shouldHold() &&
         state() != InvariantNodeState::SUBSUMED &&
         std::ranges::all_of(staticInputVarNodeIds(), [&](const VarNodeId id) {
           return invariantGraphConst()
               .varNodeConst(id)
               .definingNodes()
               .empty();
         });
}

bool TableInNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }
  invariantGraph().addImplicitConstraintNode(
      std::make_shared<TableImplicitNode>(
          invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
          std::move(_table)));
  return true;
}

void TableInNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  if (shouldHold()) {
    registerViolation(solver, mapping);
  } else {
    assert(!isReified());
    mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
    setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                          solver, mapping.intermediateId(id()), 0),
                      mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void TableInNode::registerNode(propagation::SolverBase& solver,
                               SolverMapping& mapping) const {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(shouldHold() || mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> inputVarIds;
  inputVarIds.reserve(staticInputVarNodeIds().size());
  for (const VarNodeId inputVarNodeId : staticInputVarNodeIds()) {
    assert(mapping.solverId(inputVarNodeId).isVar());
    inputVarIds.emplace_back(mapping.solverId(inputVarNodeId));
  }

  if (_isBoolTable) {
    std::vector<std::vector<bool>> boolTable(
        _table.size(), std::vector<bool>(inputVarIds.size(), false));
    for (size_t r = 0; r < _table.size(); ++r) {
      for (size_t c = 0; c < _table[r].size(); ++c) {
        boolTable[r][c] = _table[r][c] == 0;
      }
    }
    solver.makeViolationInvariant<propagation::BoolTableIn>(
        solver,
        mapping.intermediateId(id()) != propagation::NULL_ID
            ? mapping.intermediateId(id())
            : violationVarId(mapping),
        std::move(inputVarIds), boolTable);
  } else {
    solver.makeViolationInvariant<propagation::TableIn>(
        solver,
        mapping.intermediateId(id()) != propagation::NULL_ID
            ? mapping.intermediateId(id())
            : violationVarId(mapping),
        std::move(inputVarIds), _table);
  }
}

std::string TableInNode::dotLangIdentifier() const { return {"tableIn"}; }

}  // namespace atlantis::invariantgraph
