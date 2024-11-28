#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/propagation/invariants/element2dVar.hpp"

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
    IInvariantGraph& graph, VarNodeId rowIndex, VarNodeId colIndex,
    std::vector<VarNodeId>&& flatVarMatrix, VarNodeId output, size_t numCols,
    Int rowOffset, Int colOffset)
    : InvariantNode(graph, {output}, {rowIndex, colIndex},
                    std::move(flatVarMatrix)),
      _numCols(numCols),
      _rowOffset(rowOffset),
      _colOffset(colOffset) {}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    IInvariantGraph& graph, VarNodeId rowIndex, VarNodeId colIndex,
    const std::vector<std::vector<VarNodeId>>& varMatrix, VarNodeId output,
    Int rowOffset, Int colOffset)
    : ArrayVarElement2dNode(graph, rowIndex, colIndex, flatten(varMatrix),
                            output, varMatrix.size(), rowOffset, colOffset) {}

void ArrayVarElement2dNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId node) {
                    return invariantGraphConst().varNodeConst(node).isIntVar();
                  }));
  assert(
      std::all_of(dynamicInputVarNodeIds().begin(),
                  dynamicInputVarNodeIds().end(), [&](const VarNodeId node) {
                    return invariantGraph()
                               .varNodeConst(outputVarNodeIds().front())
                               .isIntVar() ==
                           invariantGraphConst().varNodeConst(node).isIntVar();
                  }));
}

size_t ArrayVarElement2dNode::flatIndex(Int row, Int col,
                                        bool addOffsets) const {
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

VarNodeId ArrayVarElement2dNode::at(Int row, Int col, bool addOffsets) const {
  return dynamicInputVarNodeIds().at(flatIndex(row, col, addOffsets));
}

void ArrayVarElement2dNode::updateState() {
  VarNode& rowIndexNode = invariantGraph().varNode(rowIndex());
  // rowIndexNode.removeValuesBelow(_rowOffset);
  // rowIndexNode.removeValuesAbove(_rowOffset + static_cast<Int>(_numCols) -
  // 1);

  VarNode& colIndexNode = invariantGraph().varNode(colIndex());
  // colIndexNode.removeValuesBelow(_colOffset);
  // colIndexNode.removeValuesAbove(_colOffset + static_cast<Int>(numCols()) -
  // 1);

  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(dynamicInputVarNodeIds().size());

  VarNodeId placeholder{NULL_NODE_ID};

  for (Int row = _rowOffset; row < _rowOffset + static_cast<Int>(numRows());
       ++row) {
    const bool inDom1 = rowIndexNode.inDomain(row);
    for (Int col = _colOffset; col < _colOffset + static_cast<Int>(_numCols);
         ++col) {
      if (inDom1 && colIndexNode.inDomain(col)) {
        continue;
      }
      if (invariantGraph().varNodeConst(at(row, col)).isFixed()) {
        continue;
      }

      varNodeIdsToRemove.emplace_back(at(row, col));

      if (placeholder == NULL_NODE_ID) {
        placeholder = invariantGraphConst()
                              .varNodeConst(outputVarNodeIds().front())
                              .isIntVar()
                          ? invariantGraph().retrieveIntVarNode(0)
                          : invariantGraph().retrieveBoolVarNode(false);
      }
      _dynamicInputVarNodeIds.at(flatIndex(row, col)) = placeholder;
    }
  }

  if (placeholder != NULL_NODE_ID &&
      std::none_of(
          invariantGraphConst()
              .varNodeConst(placeholder)
              .dynamicInputTo()
              .begin(),
          invariantGraphConst()
              .varNodeConst(placeholder)
              .dynamicInputTo()
              .end(),
          [&](const InvariantNodeId& invId) { return invId == id(); })) {
    invariantGraph().varNode(placeholder).markAsInputFor(id(), false);
  }

  for (const auto& vId : varNodeIdsToRemove) {
    if (std::none_of(dynamicInputVarNodeIds().begin(),
                     dynamicInputVarNodeIds().end(),
                     [&](const VarNodeId dId) { return dId == vId; })) {
      removeDynamicInputVarNode(vId);
    }
  }
}

void ArrayVarElement2dNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front());
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

bool ArrayVarElement2dNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (invariantGraphConst().varNodeConst(rowIndex()).isFixed() ||
          invariantGraphConst().varNodeConst(colIndex()).isFixed());
}

bool ArrayVarElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  auto& rowIndexNode = invariantGraph().varNode(rowIndex());
  auto& colIndexNode = invariantGraph().varNode(colIndex());
  assert(rowIndexNode.isFixed() || colIndexNode.isFixed());
  if (rowIndexNode.isFixed() && colIndexNode.isFixed()) {
    const VarNodeId input =
        at(rowIndexNode.lowerBound(), colIndexNode.lowerBound());
    invariantGraph().replaceVarNode(outputVarNodeIds().front(), input);
  } else if (rowIndexNode.isFixed()) {
    std::vector<VarNodeId> colArray(_numCols);
    for (Int i = 0; i < static_cast<Int>(_numCols); ++i) {
      colArray[i] = at(rowIndexNode.lowerBound(), i + _colOffset);
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), colIndex(), std::move(colArray),
        outputVarNodeIds().front(), _colOffset));
  } else {
    assert(colIndexNode.isFixed());
    std::vector<VarNodeId> rowArray(numRows());
    for (Int i = 0; i < static_cast<Int>(numRows()); ++i) {
      rowArray[i] = at(i + _rowOffset, colIndexNode.lowerBound());
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), rowIndex(), std::move(rowArray),
        outputVarNodeIds().front(), _rowOffset));
  }
  return true;
}

void ArrayVarElement2dNode::registerNode() {
  std::vector<std::vector<propagation::VarViewId>> varMatrix(
      numRows(), std::vector<propagation::VarViewId>{});
  for (size_t r = 0; r < numRows(); ++r) {
    varMatrix.at(r).reserve(_numCols);
    for (size_t c = 0; c < _numCols; ++c) {
      varMatrix.at(r).emplace_back(
          invariantGraph().varNode(at(r, c, true)).varId());
    }
  }

  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());
  solver().makeInvariant<propagation::Element2dVar>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(rowIndex()), invariantGraph().varId(colIndex()),
      std::move(varMatrix), _rowOffset, _colOffset);
}

}  // namespace atlantis::invariantgraph
