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

VarNodeId ArrayVarElement2dNode::at(Int row, Int col) const {
  assert(row >= _rowOffset && col >= _colOffset);
  const auto r = static_cast<size_t>(row - _rowOffset);
  assert(r < _numRows);
  const auto c = static_cast<size_t>(col - _colOffset);
  assert(c < numCols());
  const size_t pos = r * numCols() + c;
  assert(pos < dynamicInputVarNodeIds().size());
  return dynamicInputVarNodeIds().at(pos);
}

void ArrayVarElement2dNode::updateState() {
  VarNode& rowIdxNode = invariantGraph().varNode(rowIdx());
  rowIdxNode.removeValuesBelow(_rowOffset);
  rowIdxNode.removeValuesAbove(_rowOffset + static_cast<Int>(_numRows) - 1);

  VarNode& colIdxNode = invariantGraph().varNode(colIdx());
  colIdxNode.removeValuesBelow(_colOffset);
  colIdxNode.removeValuesAbove(_colOffset + static_cast<Int>(numCols()) - 1);

  const Int rowOverflow =
      _rowOffset + static_cast<Int>(_numRows) - 1 - rowIdxNode.upperBound();

  const Int rowUnderflow = rowIdxNode.lowerBound() - _rowOffset;

  const Int colOverflow =
      _colOffset + static_cast<Int>(numCols()) - 1 - colIdxNode.upperBound();

  const Int colUnderflow = colIdxNode.lowerBound() - _colOffset;

  if (rowOverflow <= 0 && rowUnderflow <= 0 && colOverflow <= 0 &&
      colUnderflow <= 0) {
    return;
  }

  _numRows = rowIdxNode.upperBound() - _rowOffset + 1;
  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(static_cast<Int>(dynamicInputVarNodeIds().size() -
                             _numRows * numCols()));

  Int index = 0;
  for (Int row = rowIdxNode.lowerBound(); row <= rowIdxNode.upperBound();
       ++row) {
    for (Int col = colIdxNode.lowerBound(); col <= colIdxNode.upperBound();
         ++col) {
      varNodeIdsToRemove.emplace_back(_dynamicInputVarNodeIds.at(index));
      _dynamicInputVarNodeIds.at(index) = at(row, col);
      ++index;
    }
  }

  for (auto i = static_cast<size_t>(index); i < dynamicInputVarNodeIds().size();
       ++i) {
    varNodeIdsToRemove.emplace_back(dynamicInputVarNodeIds().at(i));
  }
  for (const auto& removedVarId : varNodeIdsToRemove) {
    const bool shouldBeRemoved =
        std::none_of(dynamicInputVarNodeIds().begin(),
                     dynamicInputVarNodeIds().begin() + index,
                     [&](const VarNodeId vId) { return vId == removedVarId; });
    if (shouldBeRemoved) {
      removeDynamicInputVarNode(removedVarId);
    }
  }
  _dynamicInputVarNodeIds.resize(index);
  assert(index == static_cast<Int>(_numRows * numCols()));
}

void ArrayVarElement2dNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front());
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
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
  const auto& rowDom =
      invariantGraphConst().varNodeConst(rowIdx()).constDomain();
  const auto& colDom =
      invariantGraphConst().varNodeConst(colIdx()).constDomain();
  return std::all_of(rowDom->begin(), rowDom->end(), [&](const Int r) {
    return std::all_of(colDom->begin(), colDom->end(), [&](const Int c) {
      return invariantGraphConst().varNodeConst(at(r, c)).isFixed();
    });
  });
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
      outputVarNodeIds().front(), _rowOffset, _colOffset));
  return true;
}

void ArrayVarElement2dNode::registerNode() {
  std::vector<std::vector<propagation::VarViewId>> varMatrix(
      _numRows, std::vector<propagation::VarViewId>{});
  for (size_t r = 0; r < _numRows; ++r) {
    varMatrix.at(r).reserve(numCols());
    for (size_t c = 0; c < numCols(); ++c) {
      varMatrix.at(r).emplace_back(invariantGraph().varId(at(
          static_cast<Int>(r) + _rowOffset, static_cast<Int>(c) + _colOffset)));
    }
  }

  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());
  solver().makeInvariant<propagation::Element2dVar>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(rowIdx()), invariantGraph().varId(colIdx()),
      std::move(varMatrix), _rowOffset, _colOffset);
}

std::string ArrayVarElement2dNode::dotLangIdentifier() const {
  return "var_element2d";
}

}  // namespace atlantis::invariantgraph
