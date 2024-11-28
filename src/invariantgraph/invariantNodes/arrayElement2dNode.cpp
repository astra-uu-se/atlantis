#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"

namespace atlantis::invariantgraph {

static std::vector<Int> toFlatIntMatrix(
    const std::vector<std::vector<bool>>& boolMatrix) {
  assert(!boolMatrix.empty() && !boolMatrix.front().empty());
  assert(
      std::all_of(boolMatrix.begin(), boolMatrix.end(), [&](const auto& row) {
        return row.size() == boolMatrix.front().size();
      }));
  std::vector<Int> flatIntMatrix(boolMatrix.size() * boolMatrix.front().size());
  for (size_t r = 0; r < boolMatrix.size(); ++r) {
    for (size_t c = 0; c < boolMatrix[r].size(); ++c) {
      const size_t pos = r * boolMatrix[r].size() + c;
      flatIntMatrix[pos] = boolMatrix[r][c] ? 0 : 1;
    }
  }
  return flatIntMatrix;
}

static std::vector<Int> toFlatIntMatrix(
    const std::vector<bool>& flatBoolMatrix) {
  std::vector<Int> flatIntMatrix(flatBoolMatrix.size());
  for (size_t i = 0; i < flatBoolMatrix.size(); ++i) {
    flatIntMatrix[i] = flatBoolMatrix[i] ? 0 : 1;
  }
  return flatIntMatrix;
}

static std::vector<Int> toFlatMatrix(
    const std::vector<std::vector<Int>>& intMatrix) {
  assert(!intMatrix.empty() && !intMatrix.front().empty());
  assert(std::all_of(intMatrix.begin(), intMatrix.end(), [&](const auto& row) {
    return row.size() == intMatrix.front().size();
  }));
  std::vector<Int> flatMatrix(intMatrix.size() * intMatrix.front().size());
  for (size_t r = 0; r < intMatrix.size(); ++r) {
    for (size_t c = 0; c < intMatrix[r].size(); ++c) {
      const size_t pos = r * intMatrix[r].size() + c;
      flatMatrix[pos] = intMatrix[r][c];
    }
  }
  return flatMatrix;
}

ArrayElement2dNode::ArrayElement2dNode(IInvariantGraph& graph,
                                       VarNodeId rowIndex, VarNodeId colIndex,
                                       std::vector<Int>&& flatParMatrix,
                                       VarNodeId output, size_t numCols,
                                       Int rowOffset, Int colOffset)
    : InvariantNode(graph, {output}, {rowIndex, colIndex}),
      _flatParMatrix(std::move(flatParMatrix)),
      _numCols(static_cast<Int>(numCols)),
      _rowOffset(rowOffset),
      _colOffset(colOffset),
      _isIntMatrix(true) {}

ArrayElement2dNode::ArrayElement2dNode(
    IInvariantGraph& graph, VarNodeId rowIndex, VarNodeId colIndex,
    const std::vector<std::vector<Int>>& parMatrix, VarNodeId output,
    Int rowOffset, Int colOffset)
    : ArrayElement2dNode(graph, rowIndex, colIndex, toFlatMatrix(parMatrix),
                         output, parMatrix.front().size(), rowOffset,
                         colOffset) {}

ArrayElement2dNode::ArrayElement2dNode(
    IInvariantGraph& graph, VarNodeId rowIndex, VarNodeId colIndex,
    const std::vector<std::vector<bool>>& parMatrix, VarNodeId output,
    Int rowOffset, Int colOffset)
    : InvariantNode(graph, {output}, {rowIndex, colIndex}),
      _flatParMatrix(toFlatIntMatrix(parMatrix)),
      _numCols(parMatrix.front().size()),
      _rowOffset(rowOffset),
      _colOffset(colOffset),
      _isIntMatrix(false) {}

ArrayElement2dNode::ArrayElement2dNode(IInvariantGraph& graph,
                                       VarNodeId rowIndex, VarNodeId colIndex,
                                       const std::vector<bool>& flatMatrix,
                                       VarNodeId output, size_t numCols,
                                       Int rowOffset, Int colOffset)
    : InvariantNode(graph, {output}, {rowIndex, colIndex}),
      _flatParMatrix(toFlatIntMatrix(flatMatrix)),
      _numCols(numCols),
      _rowOffset(rowOffset),
      _colOffset(colOffset),
      _isIntMatrix(false) {}

void ArrayElement2dNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(_isIntMatrix == invariantGraphConst()
                             .varNodeConst(outputVarNodeIds().front())
                             .isIntVar());
}

size_t ArrayElement2dNode::flatIndex(Int row, Int col, bool addOffsets) const {
  if (addOffsets) {
    row += _rowOffset;
    col += _colOffset;
  }
  assert(row >= _rowOffset && col >= _colOffset);
  const size_t r = static_cast<size_t>(row - _rowOffset);
  assert(r < numRows());
  const size_t c = static_cast<size_t>(col - _colOffset);
  assert(c < _numCols);
  return r * _numCols + c;
}

Int ArrayElement2dNode::at(Int row, Int col, bool addOffsets) const {
  return _flatParMatrix.at(flatIndex(row, col, addOffsets));
}

void ArrayElement2dNode::updateState() {
  const auto& rowIndexNode = invariantGraphConst().varNodeConst(rowIndex());
  const auto& colIndexNode = invariantGraphConst().varNodeConst(colIndex());
  if (rowIndexNode.isFixed() && colIndexNode.isFixed()) {
    auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());
    if (outputNode.isIntVar()) {
      outputNode.fixToValue(
          at(rowIndexNode.lowerBound(), colIndexNode.lowerBound()));
    } else {
      outputNode.fixToValue(
          at(rowIndexNode.lowerBound(), colIndexNode.lowerBound()) == 0);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayElement2dNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (invariantGraphConst().varNodeConst(rowIndex()).isFixed() ||
          invariantGraphConst().varNodeConst(colIndex()).isFixed());
}

bool ArrayElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (invariantGraph().varNode(rowIndex()).isFixed()) {
    const Int rowIndexVal = invariantGraph().varNode(rowIndex()).lowerBound();
    assert(rowIndexVal >= _rowOffset);
    assert(rowIndexVal < _rowOffset + static_cast<Int>(numRows()));

    std::vector<Int> colArray(_numCols);
    for (Int i = _colOffset; i < static_cast<Int>(_numCols); ++i) {
      colArray[i] = at(rowIndexVal, i + _colOffset);
    }

    invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
        invariantGraph(), std::move(colArray), colIndex(),
        outputVarNodeIds().front(), _colOffset, _isIntMatrix));
    _flatParMatrix.clear();
    return true;
  }
  const Int colIndexVal = invariantGraph().varNode(colIndex()).lowerBound();
  assert(colIndexVal >= _colOffset);
  assert(colIndexVal < _colOffset + static_cast<Int>(_numCols));

  std::vector<Int> rowArray(numRows());
  for (Int i = _rowOffset; i < static_cast<Int>(numRows()); ++i) {
    rowArray[i] = at(i + _rowOffset, colIndexVal);
  }
  _flatParMatrix.clear();
  invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
      invariantGraph(), std::move(rowArray), rowIndex(),
      outputVarNodeIds().front(), _rowOffset, _isIntMatrix));
  return true;
}

void ArrayElement2dNode::registerOutputVars() {
  if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(outputVarNodeIds().front());
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayElement2dNode::registerNode() {
  if (staticInputVarNodeIds().empty()) {
    return;
  }

  std::vector<std::vector<Int>> parMatrix(numRows(),
                                          std::vector<Int>(_numCols));
  for (size_t r = 0; r < numRows(); ++r) {
    for (size_t c = 0; c < _numCols; ++c) {
      parMatrix.at(r).at(c) = at(r, c, true);
    }
  }

  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  solver().makeInvariant<propagation::Element2dConst>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(rowIndex()), invariantGraph().varId(colIndex()),
      std::move(parMatrix), _rowOffset, _colOffset);
}

}  // namespace atlantis::invariantgraph
