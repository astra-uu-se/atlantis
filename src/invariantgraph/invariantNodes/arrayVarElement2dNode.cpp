#include "invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

#include "../parseHelper.hpp"

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

void ArrayVarElement2dNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
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