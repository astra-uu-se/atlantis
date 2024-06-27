#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/times.hpp"

namespace atlantis::invariantgraph {

IntTimesNode::IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

void IntTimesNode::updateState(InvariantGraph& graph) {
  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& varNodeId : staticInputVarNodeIds()) {
    if (graph.varNodeConst(varNodeId).isFixed()) {
      varNodeIdsToRemove.emplace_back(varNodeId);
      _scalar *= graph.varNodeConst(varNodeId).lowerBound();
    }
  }

  if (_scalar == 0) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(Int{0});
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& varNodeId : varNodeIdsToRemove) {
    removeStaticInputVarNode(graph.varNode(varNodeId));
  }

  if (staticInputVarNodeIds().empty()) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(_scalar);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntTimesNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1;
}

bool IntTimesNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }

  if (staticInputVarNodeIds().empty()) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(_scalar);
    setState(InvariantNodeState::SUBSUMED);
    return true;
  }

  graph.addInvariantNode(std::make_unique<IntScalarNode>(
      staticInputVarNodeIds().front(), outputVarNodeIds().front(), _scalar, 0));
  return true;
}

void IntTimesNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntTimesNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Times>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(staticInputVarNodeIds().front()),
      invariantGraph.varId(staticInputVarNodeIds().back()));
}

}  // namespace atlantis::invariantgraph
