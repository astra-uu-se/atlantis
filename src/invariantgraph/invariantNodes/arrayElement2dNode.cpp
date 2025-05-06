#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

Int getValue(const std::vector<std::vector<Int>>& matrix, Int row, Int col,
             Int rowOffset, Int colOffset) {
  return matrix.at(row - rowOffset).at(col - colOffset);
}

static std::vector<std::vector<Int>> toIntMatrix(
    std::vector<std::vector<bool>>&& boolMatrix) {
  std::vector<std::vector<Int>> intMatrix;
  intMatrix.reserve(boolMatrix.size());
  for (auto& row : boolMatrix) {
    intMatrix.emplace_back();
    intMatrix.back().reserve(row.size());
    for (const bool par : row) {
      intMatrix.back().emplace_back(par ? 0 : 1);
    }
  }
  return intMatrix;
}

ArrayElement2dNode::ArrayElement2dNode(
    InvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
    std::vector<std::vector<Int>>&& parMatrix, VarNodeId output, Int offset1,
    Int offset2)
    : InvariantNode(graph, {output}, {idx1, idx2}),
      _parMatrix(std::move(parMatrix)),
      _rowOffset(offset1),
      _colOffset(offset2),
      _isIntMatrix(true) {}

ArrayElement2dNode::ArrayElement2dNode(
    InvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
    std::vector<std::vector<bool>>&& parMatrix, VarNodeId output, Int offset1,
    Int offset2)
    : InvariantNode(graph, {output}, {idx1, idx2}),
      _parMatrix(toIntMatrix(std::move(parMatrix))),
      _rowOffset(offset1),
      _colOffset(offset2),
      _isIntMatrix(false) {}

void ArrayElement2dNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(_isIntMatrix == invariantGraphConst()
                             .varNodeConst(outputVarNodeIds().front())
                             .isIntVar());
}

void ArrayElement2dNode::updateState() {
  auto& rowNode = invariantGraph().varNode(rowIdx());
  rowNode.domain()->removeBelow(_rowOffset);
  rowNode.domain()->removeAbove(_rowOffset +
                                static_cast<Int>(_parMatrix.size()) - 1);

  auto& colNode = invariantGraph().varNode(colIdx());
  colNode.domain()->removeBelow(_colOffset);
  colNode.domain()->removeAbove(
      _colOffset + static_cast<Int>(_parMatrix.front().size()) - 1);

  auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());

  std::unordered_set<Int> rowIndices;
  rowIndices.reserve(rowNode.constDomain()->size());
  std::unordered_set<Int> colIndices;
  colIndices.reserve(colNode.constDomain()->size());
  std::unordered_set<Int> outputVals;
  outputVals.reserve(std::min(_parMatrix.size() * _parMatrix.front().size(),
                              outputNode.constDomain()->size()));

  for (auto rowIt = rowNode.constDomain()->begin();
       rowIt != rowNode.constDomain()->end(); ++rowIt) {
    for (auto colIt = colNode.constDomain()->begin();
         colIt != colNode.constDomain()->end(); ++colIt) {
      const Int val =
          getValue(_parMatrix, *rowIt, *colIt, _rowOffset, _colOffset);
      if (outputNode.isIntVar() ? outputNode.inDomain(val)
                                : outputNode.inDomain(bool{val == 0})) {
        rowIndices.emplace(*rowIt);
        colIndices.emplace(*colIt);
        outputVals.emplace(val);
      }
    }
  }
  std::vector<Int> newRowDom(rowIndices.begin(), rowIndices.end());
  std::vector<Int> newColDom(colIndices.begin(), colIndices.end());
  rowNode.domain()->intersect(newRowDom);
  colNode.domain()->intersect(newColDom);
  if (outputNode.isIntVar()) {
    std::vector<Int> newOutDom(outputVals.begin(), outputVals.end());
    outputNode.domain()->intersect(newOutDom);
  } else if (outputVals.empty()) {
    throw InconsistencyException(
        "array_bool_element2d: output has empty domain");
  } else if (outputVals.size() == 1) {
    const bool val = (*outputVals.begin()) == 0;
    outputNode.fixToValue(val);
  }

  if (rowNode.isFixed() && colNode.isFixed()) {
    const Int val = getValue(_parMatrix, rowNode.lowerBound(),
                             colNode.lowerBound(), _rowOffset, _colOffset);
    if (outputNode.isIntVar()) {
      outputNode.fixToValue(val);
    } else {
      outputNode.fixToValue(bool{val == 0});
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayElement2dNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (invariantGraphConst().varNodeConst(rowIdx()).isFixed() ||
          invariantGraphConst().varNodeConst(colIdx()).isFixed());
}

bool ArrayElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (invariantGraph().varNode(rowIdx()).isFixed()) {
    const Int rowIndex =
        invariantGraph().varNode(rowIdx()).lowerBound() - _rowOffset;
    assert(rowIndex >= 0);
    assert(rowIndex < static_cast<Int>(_parMatrix.size()));

    invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
        invariantGraph(), std::move(_parMatrix.at(rowIndex)), colIdx(),
        outputVarNodeIds().front(), _colOffset, _isIntMatrix));
    _parMatrix.clear();
    return true;
  }
  std::vector<Int> parMatrixCol;
  const Int colIndex =
      invariantGraph().varNode(colIdx()).lowerBound() - _colOffset;
  assert(colIndex >= 0);
  assert(colIndex < static_cast<Int>(_parMatrix.front().size()));
  parMatrixCol.reserve(_parMatrix.size());
  for (const std::vector<Int>& row : _parMatrix) {
    parMatrixCol.emplace_back(row.at(colIndex));
  }
  _parMatrix.clear();
  invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
      invariantGraph(), std::move(parMatrixCol), rowIdx(),
      outputVarNodeIds().front(), _rowOffset, _isIntMatrix));
  return true;
}

void ArrayElement2dNode::registerOutputVars() {
  if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(outputVarNodeIds().front());
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void ArrayElement2dNode::registerNode() {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  solver().makeInvariant<propagation::Element2dConst>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(rowIdx()), invariantGraph().varId(colIdx()),
      std::vector<std::vector<Int>>(_parMatrix), _rowOffset, _colOffset);
}

std::string ArrayElement2dNode::dotLangIdentifier() const {
  return "element2d";
}

}  // namespace atlantis::invariantgraph
