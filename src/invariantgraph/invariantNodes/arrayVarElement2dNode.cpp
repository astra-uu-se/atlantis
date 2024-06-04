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
    VarNodeId idx1, VarNodeId idx2, std::vector<VarNodeId>&& flatVarMatrix,
    VarNodeId output, size_t numRows, Int offset1, Int offset2)
    : InvariantNode({output}, {idx1, idx2}, std::move(flatVarMatrix)),
      _numRows(numRows),
      _offset1(offset1),
      _offset2(offset2) {}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    VarNodeId idx1, VarNodeId idx2,
    std::vector<std::vector<VarNodeId>>&& varMatrix, VarNodeId output,
    Int offset1, Int offset2)
    : ArrayVarElement2dNode(idx1, idx2, flatten(varMatrix), output,
                            varMatrix.size(), offset1, offset2) {}

VarNodeId ArrayVarElement2dNode::at(Int row, Int col) {
  assert(row >= _offset1 && col >= _offset2);
  const size_t r = static_cast<size_t>(row - _offset1);
  assert(r < _numRows);
  const size_t c = static_cast<size_t>(col - _offset2);
  assert(col >= 0);
  const size_t pos = r * _numRows + c;
  return dynamicInputVarNodeIds().at(pos);
}

void ArrayVarElement2dNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

bool ArrayVarElement2dNode::canBeReplaced(const InvariantGraph& graph) const {
  return graph.varNodeConst(idx1()).isFixed() ||
         graph.varNodeConst(idx2()).isFixed();
}

bool ArrayVarElement2dNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  auto& idx1Node = invariantGraph.varNode(idx1());
  auto& idx2Node = invariantGraph.varNode(idx2());
  assert(idx1Node.isFixed() || idx2Node.isFixed());
  if (idx1Node.isFixed() && idx2Node.isFixed()) {
    const VarNodeId input = at(idx1Node.lowerBound(), idx2Node.lowerBound());
    invariantGraph.replaceVarNode(outputVarNodeIds().front(), input);
    deactivate(invariantGraph);
    return true;
  } else if (idx1Node.isFixed()) {
    std::vector<VarNodeId> column;
    column.reserve(_numRows);
    for (Int i = 0; i < static_cast<Int>(_numRows); ++i) {
      column.emplace_back(at(idx1Node.lowerBound(), i + _offset2));
    }
    invariantGraph.addInvariantNode(std::make_unique<ArrayVarElementNode>(
        idx2(), std::move(column), outputVarNodeIds().front(), _offset2));
    return true;
  } else {
    assert(idx2Node.isFixed());
    Int numCols = staticInputVarNodeIds().size() / _numRows;
    std::vector<VarNodeId> row;
    row.reserve(numCols);
    for (Int i = 0; i < numCols; ++i) {
      row.emplace_back(at(i + _offset1, idx2Node.lowerBound()));
    }
    invariantGraph.addInvariantNode(std::make_unique<ArrayVarElementNode>(
        idx1(), std::move(row), outputVarNodeIds().front(), _offset1));
  }
  return true;
}

void ArrayVarElement2dNode::registerNode(InvariantGraph& invariantGraph,
                                         propagation::SolverBase& solver) {
  const size_t numCols = dynamicInputVarNodeIds().size() / _numRows;
  std::vector<std::vector<propagation::VarId>> varMatrix(
      _numRows, std::vector<propagation::VarId>(numCols));
  for (size_t i = 0; i < _numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      varMatrix.at(i).at(j) =
          invariantGraph.varNode(dynamicInputVarNodeIds().at(i * numCols + j))
              .varId();
    }
  }

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Element2dVar>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(idx1()), invariantGraph.varId(idx2()),
      std::move(varMatrix), _offset1, _offset2);
}

}  // namespace atlantis::invariantgraph
