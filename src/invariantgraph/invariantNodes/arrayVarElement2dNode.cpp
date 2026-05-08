#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

#include <algorithm>
#include <boost/locale/boundary/index.hpp>
#include <boost/multi_index/detail/uintptr_type.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/type_erasure/placeholder.hpp>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/element2dVar.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

static std::vector<VarNodeId> flatten(
    const std::vector<std::vector<VarNodeId>>& varMatrix) {
  std::vector<VarNodeId> flatVarMatrix;
  flatVarMatrix.reserve(varMatrix.size() * varMatrix.front().size());
  for (const auto& varVector : varMatrix) {
    flatVarMatrix.insert(flatVarMatrix.end(), varVector.begin(),
                         varVector.end());
  }
  return flatVarMatrix;
}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    InvariantGraph& graph, VarNodeId rowIdx, VarNodeId colIdx,
    std::vector<VarNodeId>&& flatVarMatrix, VarNodeId output, size_t numRows,
    Int rowOffset, Int colOffset)
    : InvariantNode(graph, {output}, {rowIdx, colIdx},
                    std::move(flatVarMatrix)),
      _numRows(numRows),
      _rowOffset(rowOffset),
      _colOffset(colOffset) {}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    InvariantGraph& graph, VarNodeId rowIdx, VarNodeId colIdx,
    std::vector<std::vector<VarNodeId>>&& varMatrix, VarNodeId output,
    Int rowOffset, Int colOffset)
    : ArrayVarElement2dNode(graph, rowIdx, colIdx, flatten(varMatrix), output,
                            varMatrix.size(), rowOffset, colOffset) {}

void ArrayVarElement2dNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId node) {
        return invariantGraphConst().varNodeConst(node).isIntVar();
      }));
  assert(std::ranges::all_of(
      dynamicInputVarNodeIds().begin(), dynamicInputVarNodeIds().end(),
      [&](const VarNodeId node) {
        return invariantGraph()
                   .varNodeConst(outputVarNodeIds().front())
                   .isIntVar() ==
               invariantGraphConst().varNodeConst(node).isIntVar();
      }));
}

size_t ArrayVarElement2dNode::index(const Int row, const Int col, const bool useOffset) const {
  if (useOffset) {
    assert(row >= _rowOffset && col >= _colOffset);
    const auto r = static_cast<size_t>(row - _rowOffset);
    assert(r < _numRows);
    const auto c = static_cast<size_t>(col - _colOffset);
    assert(c < numCols());
    return r * numCols() + c;
  }
  assert(0 <= row);
  assert(row < static_cast<Int>(_numRows));
  assert(0 <= col);
  assert(col < static_cast<Int>(numCols()));
  return row * numCols() + col;
}

VarNodeId ArrayVarElement2dNode::at(const Int row, const Int col, const bool useOffset) const {
  return dynamicInputVarNodeIds()[index(row, col, useOffset)];
}

void ArrayVarElement2dNode::postConstraint() {
  std::vector<std::vector<ConstraintVarId>> matrix(_numRows);
  for (size_t r = 0; r < _numRows; ++r) {
    matrix[r].reserve(numCols());
    for (size_t c = 0; c < numCols(); ++c) {
      matrix[r].emplace_back(varNodeConst(at(r, c, false)).constraintVarId());
    }
  }
  if (outputVarNodeConst(0).isIntVar()) {
    constraintSolver().array_var_int_element2d(varNodeConst(rowIdx()).constraintVarId(),varNodeConst(colIdx()).constraintVarId(), matrix, outputVarNodeConst(0).constraintVarId(), _rowOffset, _colOffset);
  } else {
    constraintSolver().array_var_bool_element2d(varNodeConst(rowIdx()).constraintVarId(),varNodeConst(colIdx()).constraintVarId(), matrix, outputVarNodeConst(0).constraintVarId(), _rowOffset, _colOffset);
  }

}

void ArrayVarElement2dNode::updateState() {
  InvariantNode::updateState();

  const Int rowLb = staticInputVarNodeConst(rowIdx()).lowerBound();
  const Int colLb = staticInputVarNodeConst(colIdx()).lowerBound();

  std::vector<size_t> indicesToRemove;
  indicesToRemove.reserve((rowLb - _rowOffset) * (colLb - _colOffset));

  // Find invalid start rows:
  for (Int r = _rowOffset; r < rowLb; ++r) {
    for (Int c = 0; c < static_cast<Int>(numCols()); ++c) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  // Find invalid end rows:
  const Int rowSize = staticInputVarNodeConst(rowIdx()).upperBound() - staticInputVarNodeConst(rowIdx()).lowerBound() + 1;
  assert(rowSize <= static_cast<Int>(_numRows));
  for (Int r = static_cast<Int>(_numRows); r > rowSize; --r) {
    for (Int c = 0; c < static_cast<Int>(numCols()); ++c) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  // Find invalid start columns:
  for (Int c = _colOffset; c < rowLb; ++c) {
    for (Int r = 0; r < static_cast<Int>(_numRows); ++r) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  // Find invalid end columns:
  const Int colSize = staticInputVarNodeConst(colIdx()).upperBound() - staticInputVarNodeConst(colIdx()).lowerBound() + 1;
  assert(colSize <= static_cast<Int>(numCols()));
  for (Int c = static_cast<Int>(numCols()); c > rowSize; --c) {
    for (Int r = 0; r < static_cast<Int>(_numRows); ++r) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  std::ranges::sort(indicesToRemove);
  const auto [first, last] = std::ranges::unique(indicesToRemove);
  indicesToRemove.erase(first, last);

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i > 0; --i) {
    removeDynamicInputAtIndex(indicesToRemove[i]);
  }
  if (varNodeConst(rowIdx()).constDomain()->isInterval() && varNodeConst(colIdx()).constDomain()->isInterval()) {
    return;
  }

  std::vector<bool> indexIsSupported(dynamicInputVarNodeIds().size(), false);

  for (auto rowIter = varNodeConst(rowIdx()).constDomain()->begin(); rowIter != varNodeConst(rowIdx()).constDomain()->end(); ++rowIter) {
    for (auto colIter = varNodeConst(rowIdx()).constDomain()->begin(); colIter != varNodeConst(rowIdx()).constDomain()->end(); ++colIter) {
      indexIsSupported[index(*rowIter, *colIter, true)] = true;
    }
  }
  VarNodeId prevVarNodeId{NULL_NODE_ID};
  for (const VarNodeId vId : dynamicInputVarNodeIds()) {
    if (vId != NULL_NODE_ID) {
      prevVarNodeId = vId;
      break;
    }
  }
  assert(prevVarNodeId != NULL_NODE_ID);
  for (size_t i = 1; i < dynamicInputVarNodeIds().size(); ++i) {
    if (indexIsSupported[i]) {
      prevVarNodeId = dynamicInputVarNodeIds()[i];
      continue;
    }
    for (size_t j = i + 1; j < dynamicInputVarNodeIds().size(); ++j) {
      if (!indexIsSupported[j] && dynamicInputVarNodeIds()[i] == dynamicInputVarNodeIds()[j]) {
        indexIsSupported[j] = true;
      }
    }
    replaceDynamicInputVarNode(staticInputVarNodeIds()[i], prevVarNodeId);
  }
}

void ArrayVarElement2dNode::registerOutputVars(propagation::SolverBase& solver,
                                               SolverMapping& mapping) const {
  makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

bool ArrayVarElement2dNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (invariantGraphConst().varNodeConst(rowIdx()).isFixed() ||
      invariantGraphConst().varNodeConst(colIdx()).isFixed()) {
    return true;
  }
  const bool allFixed = std::ranges::all_of(dynamicInputVarNodeIds(), [&](const VarNodeId vId) {
    return varNodeConst(vId).isFixed();
  });
  if (allFixed) {
    return true;
  }
  const bool allSameVar = std::ranges::all_of(dynamicInputVarNodeIds(), [&](const VarNodeId vId) {
    return vId == dynamicInputVarNodeIds().front();
  });
  if (allSameVar) {
    return true;
  }
}

bool ArrayVarElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  const auto& rowNode = invariantGraph().varNode(rowIdx());
  const auto& colNode = invariantGraph().varNode(colIdx());
  if (invariantGraphConst().varNodeConst(rowIdx()).isFixed() ||
      invariantGraphConst().varNodeConst(colIdx()).isFixed()) {
    if (rowNode.isFixed() && colNode.isFixed()) {
      const VarNodeId input = at(rowNode.lowerBound(), colNode.lowerBound());
      invariantGraph().replaceVarNode(outputVarNodeIds().front(), input);
      return true;
    }
    if (rowNode.isFixed()) {
      std::vector<VarNodeId> column;
      assert(dynamicInputVarNodeIds().size() % _numRows == 0);
      column.reserve(numCols());
      for (size_t c = 0; c < numCols(); ++c) {
        column.emplace_back(
            at(rowNode.lowerBound(), static_cast<Int>(c) + _colOffset));
      }
      invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
          invariantGraph(), colIdx(), std::move(column),
          outputVarNodeIds().front(), _colOffset));
      return true;
    }
    assert(colNode.isFixed());
    std::vector<VarNodeId> row;
    row.reserve(_numRows);
    for (size_t r = 0; r < _numRows; ++r) {
      row.emplace_back(
          at(static_cast<Int>(r) + _rowOffset, colNode.lowerBound()));
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), rowIdx(), std::move(row), outputVarNodeIds().front(),
        _rowOffset));
    return true;
  }
  assert(dynamicInputVarNodeIds().size() % _numRows == 0);
  const Int defVal =
      invariantGraph()
          .varNodeConst(at(rowNode.lowerBound(), colNode.lowerBound()))
          .lowerBound();
  std::vector<std::vector<Int>> parMatrix(_numRows,
                                          std::vector<Int>(numCols(), defVal));
  for (const Int row : *rowNode.constDomain()) {
    const Int r = row - _rowOffset;
    for (const Int col : *colNode.constDomain()) {
      const Int c = col - _colOffset;
      assert(invariantGraphConst().varNodeConst(at(row, col)).isFixed());
      parMatrix.at(r).at(c) =
          invariantGraphConst().varNodeConst(at(row, col)).lowerBound();
    }
  }
  invariantGraph().addInvariantNode(std::make_shared<ArrayElement2dNode>(
      invariantGraph(), rowIdx(), colIdx(), std::move(parMatrix),
      outputVarNodeIds().front(), _rowOffset, _colOffset,
      invariantGraphConst()
          .varNodeConst(outputVarNodeIds().front())
          .isIntVar()));
  return true;
}

void ArrayVarElement2dNode::registerNode(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  std::vector<std::vector<propagation::VarViewId>> varMatrix(
      _numRows, std::vector<propagation::VarViewId>{});
  for (size_t r = 0; r < _numRows; ++r) {
    varMatrix.at(r).reserve(numCols());
    for (size_t c = 0; c < numCols(); ++c) {
      varMatrix.at(r).emplace_back(mapping.solverId(at(
          static_cast<Int>(r) + _rowOffset, static_cast<Int>(c) + _colOffset)));
    }
  }

  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());
  solver.makeInvariant<propagation::Element2dVar>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(rowIdx()), mapping.solverId(colIdx()),
      std::move(varMatrix), _rowOffset, _colOffset);
}

std::string ArrayVarElement2dNode::dotLangIdentifier() const {
  return "var_element2d";
}

}  // namespace atlantis::invariantgraph
