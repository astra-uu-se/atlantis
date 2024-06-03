#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"

namespace atlantis::invariantgraph {

static std::vector<std::vector<Int>> toIntMatrix(
    std::vector<std::vector<bool>>&& boolMatrix) {
  std::vector<std::vector<Int>> intMatrix;
  intMatrix.reserve(boolMatrix.size());
  for (auto& row : boolMatrix) {
    intMatrix.emplace_back();
    intMatrix.back().reserve(row.size());
    for (bool par : row) {
      intMatrix.back().emplace_back(par ? 0 : 1);
    }
  }
  return intMatrix;
}

ArrayElement2dNode::ArrayElement2dNode(
    VarNodeId idx1, VarNodeId idx2, std::vector<std::vector<Int>>&& parMatrix,
    VarNodeId output, Int offset1, Int offset2)
    : InvariantNode({output}, {idx1, idx2}),
      _parMatrix(std::move(parMatrix)),
      _offset1(offset1),
      _offset2(offset2) {}

ArrayElement2dNode::ArrayElement2dNode(
    VarNodeId idx1, VarNodeId idx2, std::vector<std::vector<bool>>&& parMatrix,
    VarNodeId output, Int offset1, Int offset2)
    : ArrayElement2dNode(idx1, idx2, toIntMatrix(std::move(parMatrix)), output,
                         offset1, offset2) {}

void ArrayElement2dNode::propagate(InvariantGraph& graph) {
  if (!graph.isFixed(idx1()) && !graph.isFixed(idx2())) {
    return;
  }
  if (graph.isFixed(idx2()) && graph.isFixed(idx2())) {
    removeStaticInputVarNode(graph.varNode(idx1()));
    removeStaticInputVarNode(graph.varNode(idx2()));
    const Int val = _parMatrix[graph.lowerBound(idx1()) + _offset1]
                              [graph.lowerBound(idx2()) + _offset2];
    graph.fixToValue(outputVarNodeIds().front(), val);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  setState(InvariantNodeState::REPLACABLE);
}

bool ArrayElement2dNode::replace(InvariantGraph& invariantGraph) {
  if (state() != InvariantNodeState::REPLACABLE) {
    return false;
  }
  assert(invariantGraph.varNode(idx1()).isFixed() !=
         invariantGraph.varNode(idx2()).isFixed());
  if (invariantGraph.varNode(idx1()).isFixed()) {
    invariantGraph.replaceInvariantNode(
        id(),
        std::make_unique<ArrayElementNode>(
            std::move(_parMatrix[invariantGraph.varNode(idx1()).lowerBound() +
                                 _offset1]),
            idx2(), outputVarNodeIds().front(), _offset2));
    return true;
  }
  std::vector<Int> parMatrixRow;
  const Int col = invariantGraph.varNode(idx2()).lowerBound() + _offset2;
  parMatrixRow.reserve(_parMatrix.size());
  for (const std::vector<Int>& row : _parMatrix) {
    parMatrixRow.emplace_back(row[col]);
  }
  invariantGraph.replaceInvariantNode(
      id(),
      std::make_unique<ArrayElementNode>(std::move(parMatrixRow), idx1(),
                                         outputVarNodeIds().front(), _offset1));
  return true;
}

void ArrayElement2dNode::registerOutputVars(InvariantGraph& invariantGraph,
                                            propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayElement2dNode::registerNode(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Element2dConst>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(idx1()), invariantGraph.varId(idx2()),
      std::vector<std::vector<Int>>(_parMatrix), _offset1, _offset2);
}

}  // namespace atlantis::invariantgraph
