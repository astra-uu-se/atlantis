#include "invariantgraph/violationInvariantNode.hpp"

#include "invariantgraph/invariantGraph.hpp"

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
 * propagation engine.
 */

ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& outputIds, std::vector<VarNodeId>&& staticInputIds,
    VarNodeId reifiedId, bool shouldHold)
    : InvariantNode(std::move(combine(reifiedId, std::move(outputIds))),
                    std::move(staticInputIds)),
      _reifiedViolationNodeId(reifiedId),
      _shouldHold(shouldHold) {
  if (!isReified()) {
    assert(_reifiedViolationNodeId == NULL_NODE_ID);
  } else {
    assert(_reifiedViolationNodeId != NULL_NODE_ID);
    assert(outputVarNodeIds().front() == _reifiedViolationNodeId);
  }
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
                             NULL_NODE_ID, shouldHold) {}

ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& staticInputIds, bool shouldHold)
    : ViolationInvariantNode({}, std::move(staticInputIds), NULL_NODE_ID,
                             shouldHold) {}
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

propagation::VarId ViolationInvariantNode::setViolationVarId(InvariantGraph& invariantGraph,
                                                propagation::VarId varId) {
  assert(violationVarId(invariantGraph) == propagation::NULL_ID);
  if (isReified()) {
    invariantGraph.varNode(_reifiedViolationNodeId).setVarId(varId);
  } else {
    _violationVarId = varId;
  }
  return violationVarId(invariantGraph);
}

propagation::VarId ViolationInvariantNode::registerViolation(InvariantGraph& invariantGraph,
                                                propagation::Engine& engine,
                                                Int initialValue) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    return setViolationVarId(
        invariantGraph,
        engine.makeIntVar(initialValue, initialValue, initialValue));
  }
  return violationVarId(invariantGraph);
}

}  // namespace invariantgraph