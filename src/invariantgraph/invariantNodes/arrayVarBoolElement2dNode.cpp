#include "invariantgraph/invariantNodes/arrayVarBoolElement2dNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayVarBoolElement2dNode::ArrayVarBoolElement2dNode(
    VarNodeId idx1, VarNodeId idx2, std::vector<VarNodeId>&& x,
    VarNodeId output, size_t numRows, Int offset1, Int offset2)
    : InvariantNode({output}, {idx1, idx2}, std::move(x)),
      _numRows(numRows),
      _offset1(offset1),
      _offset2(offset2) {}

void ArrayVarBoolElement2dNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()), 1);
}

void ArrayVarBoolElement2dNode::registerNode(InvariantGraph& invariantGraph,
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
      invariantGraph.varId(idx1()), invariantGraph.varId(idx2()), varMatrix,
      _offset1, _offset2);
}

}  // namespace atlantis::invariantgraph