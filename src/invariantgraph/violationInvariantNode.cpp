#include "atlantis/invariantgraph/violationInvariantNode.hpp"

#include <cassert>

namespace atlantis::invariantgraph {

static std::vector<invariantgraph::VarNodeId> combine(
    VarNodeId reifiedId, std::vector<VarNodeId>&& outputIds) {
  if (reifiedId == NULL_NODE_ID) {
    return std::move(outputIds);
  }
  outputIds.insert(outputIds.begin(), reifiedId);
  return std::move(outputIds);
}

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation solver.
 */

ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& outputIds, std::vector<VarNodeId>&& staticInputIds,
    VarNodeId reifiedId, bool shouldHold)
    : InvariantNode(combine(reifiedId, std::move(outputIds)),
                    std::move(staticInputIds)),
      _reifiedViolationNodeId(reifiedId),
      _shouldHold(shouldHold) {
  assert((!isReified() && _reifiedViolationNodeId == NULL_NODE_ID) ||
         (_reifiedViolationNodeId != NULL_NODE_ID &&
          outputVarNodeIds().front() == _reifiedViolationNodeId));
}

bool ViolationInvariantNode::shouldHold() const noexcept { return _shouldHold; }

ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& outputIds, std::vector<VarNodeId>&& staticInputIds,
    VarNodeId reifiedId)
    : ViolationInvariantNode(std::move(outputIds), std::move(staticInputIds),
                             reifiedId, true) {}

ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& staticInputIds, VarNodeId reifiedId)
    : ViolationInvariantNode({}, std::move(staticInputIds), reifiedId, true) {}

ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& outputIds, std::vector<VarNodeId>&& staticInputIds,
    bool shouldHold)
    : ViolationInvariantNode(std::move(outputIds), std::move(staticInputIds),
                             VarNodeId(NULL_NODE_ID), shouldHold) {}

ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& staticInputIds, bool shouldHold)
    : ViolationInvariantNode({}, std::move(staticInputIds),
                             VarNodeId(NULL_NODE_ID), shouldHold) {}

bool ViolationInvariantNode::isReified() const {
  return _reifiedViolationNodeId != NULL_NODE_ID;
}

propagation::VarId ViolationInvariantNode::violationVarId(
    const InvariantGraph& invariantGraph) const {
  if (isReified()) {
    return invariantGraph.varId(_reifiedViolationNodeId);
  }
  return _violationVarId;
}

VarNodeId ViolationInvariantNode::reifiedViolationNodeId() {
  return _reifiedViolationNodeId;
}

void ViolationInvariantNode::updateState(InvariantGraph& graph) {
  if (isReified() && graph.varNodeConst(_reifiedViolationNodeId).isFixed()) {
    _shouldHold = graph.varNodeConst(_reifiedViolationNodeId).inDomain(true);
    _reifiedViolationNodeId = NULL_NODE_ID;
  }
  InvariantNode::updateState(graph);
}

propagation::VarId ViolationInvariantNode::setViolationVarId(
    InvariantGraph& invariantGraph, propagation::VarId varId) {
  assert(violationVarId(invariantGraph) == propagation::NULL_ID);
  if (isReified()) {
    invariantGraph.varNode(_reifiedViolationNodeId).setVarId(varId);
  } else {
    _violationVarId = varId;
  }
  return violationVarId(invariantGraph);
}

propagation::VarId ViolationInvariantNode::registerViolation(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver,
    Int initialValue) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    return setViolationVarId(
        invariantGraph,
        solver.makeIntVar(initialValue, initialValue, initialValue));
  }
  return violationVarId(invariantGraph);
}

}  // namespace atlantis::invariantgraph
