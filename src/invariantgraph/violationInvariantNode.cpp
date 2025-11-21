#include "atlantis/invariantgraph/violationInvariantNode.hpp"

#include <cassert>

#include "atlantis/invariantgraph/fzn/array_bool_and.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

static std::vector<VarNodeId> combine(VarNodeId reifiedId,
                                      std::vector<VarNodeId>&& outputIds) {
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
    InvariantGraph& graph, std::vector<VarNodeId>&& outputIds,
    std::vector<VarNodeId>&& staticInputIds, VarNodeId reifiedViolationId,
    bool shouldHold)
    : InvariantNode(graph, combine(reifiedViolationId, std::move(outputIds)),
                    std::move(staticInputIds)),
      _isReified(reifiedViolationId != NULL_NODE_ID),
      _shouldHold(shouldHold) {
  assert((!ViolationInvariantNode::isReified() &&
          reifiedViolationId == NULL_NODE_ID) ||
         (reifiedViolationId != NULL_NODE_ID &&
          InvariantNode::outputVarNodeIds().front() == reifiedViolationId));
}

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& outputIds,
    std::vector<VarNodeId>&& staticInputIds, VarNodeId reifiedViolationId)
    : ViolationInvariantNode(graph, std::move(outputIds),
                             std::move(staticInputIds), reifiedViolationId,
                             true) {}

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& staticInputIds,
    VarNodeId reifiedViolationId)
    : ViolationInvariantNode(graph, {}, std::move(staticInputIds),
                             reifiedViolationId, true) {}

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& outputIds,
    std::vector<VarNodeId>&& staticInputIds, bool shouldHold)
    : ViolationInvariantNode(graph, std::move(outputIds),
                             std::move(staticInputIds), VarNodeId{NULL_NODE_ID},
                             shouldHold) {}

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& staticInputIds,
    bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(staticInputIds),
                             VarNodeId{NULL_NODE_ID}, shouldHold) {}

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
      const bool isAlsoOutput = std::ranges::any_of(
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

propagation::VarViewId ViolationInvariantNode::violationVarId(
    const SolverMapping& mapping) const {
  if (isReified()) {
    return mapping.solverId(outputVarNodeIds().front());
  }
  return mapping.violationId(id());
}

VarNodeId ViolationInvariantNode::reifiedViolationNodeId() const {
  return isReified() ? outputVarNodeIds().front() : VarNodeId{NULL_NODE_ID};
}

void ViolationInvariantNode::updateState() { updateReified(); }

propagation::VarViewId ViolationInvariantNode::setViolationVarId(
    propagation::VarViewId varId, SolverMapping& mapping) const {
  if (isReified()) {
    if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
      mapping.setSolverId(outputVarNodeIds().front(), varId);
    }
    return mapping.solverId(outputVarNodeIds().front());
  }
  if (mapping.violationId(id()) == propagation::NULL_ID) {
    mapping.setViolationId(id(), varId);
  }
  return mapping.violationId(id());
}

propagation::VarViewId ViolationInvariantNode::registerViolation(
    Int initialValue, propagation::SolverBase& solver,
    SolverMapping& mapping) const {
  if (isReified()) {
    if (mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID) {
      return mapping.solverId(outputVarNodeIds().front());
    }
  } else if (mapping.violationId(id()) != propagation::NULL_ID) {
    return mapping.violationId(id());
  }
  return setViolationVarId(
      solver.makeIntVar(initialValue, initialValue, initialValue), mapping);
}

propagation::VarViewId ViolationInvariantNode::registerViolation(
    propagation::SolverBase& solver, SolverMapping& mapping) const {
  return registerViolation(0, solver, mapping);
}

}  // namespace atlantis::invariantgraph
