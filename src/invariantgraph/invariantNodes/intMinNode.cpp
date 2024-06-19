#include "atlantis/invariantgraph/invariantNodes/intMinNode.hpp"

#include <cmath>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/binaryMin.hpp"

namespace atlantis::invariantgraph {

IntMinNode::IntMinNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

void IntMinNode::updateState(InvariantGraph& graph) {
  const Int max = graph.varNodeConst(outputVarNodeIds().front()).upperBound();
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).upperBound() > max) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  if (staticInputVarNodeIds().size() == 1) {
    graph.replaceVarNode(outputVarNodeIds().front(),
                         staticInputVarNodeIds().front());
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntMinNode::registerOutputVars(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntMinNode::registerNode(InvariantGraph& invariantGraph,
                              propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::BinaryMin>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

}  // namespace atlantis::invariantgraph
