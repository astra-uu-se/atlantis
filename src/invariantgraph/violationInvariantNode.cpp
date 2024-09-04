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
 * Serves as a marker for the invariant invariantGraph() to start the
 * application to the propagation solver.
 */

ViolationInvariantNode::ViolationInvariantNode(
    IInvariantGraph& graph, std::vector<VarNodeId>&& outputIds,
    std::vector<VarNodeId>&& staticInputIds, VarNodeId reifiedId,
    bool shouldHold)
    : InvariantNode(graph, combine(reifiedId, std::move(outputIds)),
                    std::move(staticInputIds)),
      _isReified(reifiedId != NULL_NODE_ID),
      _shouldHold(shouldHold) {
  assert(
      (!isReified() && reifiedId == NULL_NODE_ID) ||
      (reifiedId != NULL_NODE_ID && outputVarNodeIds().front() == reifiedId));
}

ViolationInvariantNode::ViolationInvariantNode(
    IInvariantGraph& graph, std::vector<VarNodeId>&& outputIds,
    std::vector<VarNodeId>&& staticInputIds, VarNodeId reifiedId)
    : ViolationInvariantNode(graph, std::move(outputIds),
                             std::move(staticInputIds), reifiedId, true) {}

ViolationInvariantNode::ViolationInvariantNode(
    IInvariantGraph& graph, std::vector<VarNodeId>&& staticInputIds,
    VarNodeId reifiedId)
    : ViolationInvariantNode(graph, {}, std::move(staticInputIds), reifiedId,
                             true) {}

ViolationInvariantNode::ViolationInvariantNode(
    IInvariantGraph& graph, std::vector<VarNodeId>&& outputIds,
    std::vector<VarNodeId>&& staticInputIds, bool shouldHold)
    : ViolationInvariantNode(graph, std::move(outputIds),
                             std::move(staticInputIds), VarNodeId(NULL_NODE_ID),
                             shouldHold) {}

ViolationInvariantNode::ViolationInvariantNode(
    IInvariantGraph& graph, std::vector<VarNodeId>&& staticInputIds,
    bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(staticInputIds),
                             VarNodeId(NULL_NODE_ID), shouldHold) {}

void ViolationInvariantNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
}

bool ViolationInvariantNode::shouldHold() const noexcept { return _shouldHold; }

void ViolationInvariantNode::fixReified(bool shouldHold) {
  if (isReified()) {
    invariantGraph().varNode(reifiedViolationNodeId()).fixToValue(shouldHold);
    updateReified();
  }
}

bool ViolationInvariantNode::isReified() const { return _isReified; }

void ViolationInvariantNode::updateReified() {
  if (isReified() &&
      invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isFixed()) {
    _shouldHold = invariantGraph()
                      .varNodeConst(reifiedViolationNodeId())
                      .inDomain(bool{true});
    if (!outputVarNodeIds().empty()) {
      assert(outputVarNodeIds().front() == reifiedViolationNodeId());
      const bool isAlsoOutput = std::any_of(
          outputVarNodeIds().begin() + 1, outputVarNodeIds().end(),
          [this](VarNodeId oId) { return oId == reifiedViolationNodeId(); });
      if (!isAlsoOutput) {
        removeOutputVarNode(reifiedViolationNodeId());
      }
    }
    _isReified = false;
  }
  InvariantNode::updateState();
}

propagation::VarId ViolationInvariantNode::violationVarId() const {
  if (isReified()) {
    return invariantGraphConst().varId(outputVarNodeIds().front());
  }
  return _violationVarId;
}

VarNodeId ViolationInvariantNode::reifiedViolationNodeId() {
  return isReified() ? outputVarNodeIds().front() : VarNodeId{NULL_NODE_ID};
}

void ViolationInvariantNode::updateState() { updateReified(); }

propagation::VarId ViolationInvariantNode::setViolationVarId(
    propagation::VarId varId) {
  assert(violationVarId() == propagation::NULL_ID);
  if (isReified()) {
    invariantGraph().varNode(outputVarNodeIds().front()).setVarId(varId);
  } else {
    _violationVarId = varId;
  }
  return violationVarId();
}

propagation::VarId ViolationInvariantNode::registerViolation(Int initialValue) {
  if (violationVarId() == propagation::NULL_ID) {
    return setViolationVarId(
        solver().makeIntVar(initialValue, initialValue, initialValue));
  }
  return violationVarId();
}

}  // namespace atlantis::invariantgraph
