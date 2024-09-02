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
    InvariantGraph& graph, std::vector<VarNodeId>&& outputIds,
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

void ViolationInvariantNode::init(const InvariantNodeId& id) {
  InvariantNode::init(id);
}

bool ViolationInvariantNode::shouldHold() const noexcept { return _shouldHold; }

void ViolationInvariantNode::fixReified(InvariantGraph& graph,
                                        bool shouldHold) {
  if (isReified()) {
    graph.varNode(reifiedViolationNodeId()).fixToValue(shouldHold);
    updateReified(graph);
  }
}

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

bool ViolationInvariantNode::isReified() const { return _isReified; }

void ViolationInvariantNode::updateReified(InvariantGraph& graph) {
  if (isReified() && graph.varNodeConst(reifiedViolationNodeId()).isFixed()) {
    _shouldHold =
        graph.varNodeConst(reifiedViolationNodeId()).inDomain(bool{true});
    if (!outputVarNodeIds().empty()) {
      assert(outputVarNodeIds().front() == reifiedViolationNodeId());
      const bool isAlsoOutput = std::any_of(
          outputVarNodeIds().begin() + 1, outputVarNodeIds().end(),
          [this](VarNodeId oId) { return oId == reifiedViolationNodeId(); });
      if (!isAlsoOutput) {
        removeOutputVarNode(graph.varNode(reifiedViolationNodeId()));
      }
    }
    _isReified = false;
  }
  InvariantNode::updateState(graph);
}

propagation::VarId ViolationInvariantNode::violationVarId(
    const InvariantGraph& graph) const {
  if (isReified()) {
    return graph.varId(outputVarNodeIds().front());
  }
  return _violationVarId;
}

VarNodeId ViolationInvariantNode::reifiedViolationNodeId() {
  return isReified() ? outputVarNodeIds().front() : VarNodeId{NULL_NODE_ID};
}

void ViolationInvariantNode::updateState() { updateReified(graph); }

propagation::VarId ViolationInvariantNode::setViolationVarId(
    InvariantGraph& graph, propagation::VarId varId) {
  assert(violationVarId(graph) == propagation::NULL_ID);
  if (isReified()) {
    graph.varNode(outputVarNodeIds().front()).setVarId(varId);
  } else {
    _violationVarId = varId;
  }
  return violationVarId(graph);
}

propagation::VarId ViolationInvariantNode::registerViolation(
    InvariantGraph& graph, propagation::SolverBase& solver, Int initialValue) {
  if (violationVarId(graph) == propagation::NULL_ID) {
    return setViolationVarId(
        graph, solver.makeIntVar(initialValue, initialValue, initialValue));
  }
  return violationVarId(graph);
}

}  // namespace atlantis::invariantgraph
