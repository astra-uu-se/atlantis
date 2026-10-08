#include "atlantis/invariantgraph/violationInvariantNode.hpp"

#include <cassert>

#include "atlantis/invariantgraph/fzn/array_bool_and.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

static std::vector<std::shared_ptr<VarNode>> combine(const std::shared_ptr<VarNode>& reified,
                                      std::vector<std::shared_ptr<VarNode>>&& outputs) {
  if (reified == nullptr) {
    return std::move(outputs);
  }
  outputs.insert(outputs.begin(), reified);
  return std::move(outputs);
}

/**
 * Serves as a marker for the invariant invariantGraph() to start the
 * application to the propagation solver.
 */

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& outputs,
    std::vector<std::shared_ptr<VarNode>>&& staticInputs, const std::shared_ptr<VarNode>& reifiedViolation,
    bool shouldHold)
    : InvariantNode(graph, combine(reifiedViolation, std::move(outputs)),
                    std::move(staticInputs)),
      _isReified(reifiedViolation != nullptr),
      _shouldHold(shouldHold) {
  assert((!ViolationInvariantNode::isReified() &&
          reifiedViolation == nullptr) ||
         (reifiedViolation != nullptr &&
          InvariantNode::outputVarNodes().front() == reifiedViolation));
}

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& outputs,
    std::vector<std::shared_ptr<VarNode>>&& staticInputs, VarNode& reifiedViolation)
    : ViolationInvariantNode(graph, std::move(outputs),
                             std::move(staticInputs), reifiedViolation.ptr(),
                             true) {}

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& staticInputs,
    VarNode& reifiedViolation)
    : ViolationInvariantNode(graph, {}, std::move(staticInputs),
                             reifiedViolation.ptr(), true) {}

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& outputs,
    std::vector<std::shared_ptr<VarNode>>&& staticInputs,
    const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(outputs),
                             std::move(staticInputs), nullptr,
                             shouldHold) {}

ViolationInvariantNode::ViolationInvariantNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& staticInputs,
    bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(staticInputs),
                             nullptr, shouldHold) {}

void ViolationInvariantNode::init() {
  InvariantNode::init();
}

bool ViolationInvariantNode::shouldHold() const noexcept { return _shouldHold; }

void ViolationInvariantNode::setShouldHold(const bool sh) noexcept {
  _shouldHold = sh;
}

void ViolationInvariantNode::fixReified(bool shouldHold) {
  if (isReified()) {
    reifiedViolationNode()->fixToValue(shouldHold);
    updateReified();
  }
}

bool ViolationInvariantNode::isReified() const { return _isReified; }

bool ViolationInvariantNode::isViolationInvariant() const {
  return !isReified();
}

void ViolationInvariantNode::updateReified() {
  if (isReified() &&
      reifiedViolationNode()->isFixed()) {
    _shouldHold = reifiedViolationNode()->inDomain(bool{true});
    const auto& reifViol = reifiedViolationNode();
    // _isReified must be changed *before* removing the output variable
    _isReified = false;
    if (!outputVarNodes().empty()) {
      assert(outputVarNodes().front() == reifViol);
      const bool isAlsoOutput = std::ranges::any_of(
          outputVarNodes().begin() + 1, outputVarNodes().end(),
          [reifViol](const auto& other) { return other == reifViol; });
      if (!isAlsoOutput) {
        removeOutputVarNode(*reifViol);
      }
    }
  }
  InvariantNode::updateState();
}

propagation::VarViewId ViolationInvariantNode::violationVarId(
    const SolverMapping& mapping) const {
  if (isReified()) {
    return mapping.solverId(outputVarNodes().front()->mappingId());
  }
  return mapping.violationId(TODO);
}

std::shared_ptr<VarNode> ViolationInvariantNode::reifiedViolationNode() {
  return isReified() ? outputVarNodes().front() : std::shared_ptr<VarNode>{nullptr};
}

void ViolationInvariantNode::postConstraint() { updateReified(); }

void ViolationInvariantNode::updateState() { updateReified(); }

bool ViolationInvariantNode::constrainsOutput(const VarNode&) const {
  return !isReified();
}

propagation::VarViewId ViolationInvariantNode::setViolationVarId(
    const propagation::VarViewId varId, SolverMapping& mapping) const {
  if (isReified()) {
    if (mapping.solverId(outputVarNodes().front()->mappingId()) == propagation::NULL_ID) {
      mapping.setSolverId(outputVarNodes().front()->mappingId(), varId);
    }
    return mapping.solverId(outputVarNodes().front()->mappingId());
  }
  if (mapping.violationId(TODO) == propagation::NULL_ID) {
    mapping.setViolationId(TODO, varId);
  }
  return mapping.violationId(TODO);
}

propagation::VarViewId ViolationInvariantNode::registerViolation(
    Int initialValue, propagation::SolverBase& solver,
    SolverMapping& mapping) const {
  if (isReified()) {
    if (mapping.solverId(outputVarNodes().front()->mappingId()) != propagation::NULL_ID) {
      return mapping.solverId(outputVarNodes().front()->mappingId());
    }
  } else if (mapping.violationId(TODO) != propagation::NULL_ID) {
    return mapping.violationId(TODO);
  }
  return setViolationVarId(
      solver.makeIntVar(initialValue, initialValue, initialValue), mapping);
}

propagation::VarViewId ViolationInvariantNode::registerViolation(
    propagation::SolverBase& solver, SolverMapping& mapping) const {
  return registerViolation(0, solver, mapping);
}

}  // namespace atlantis::invariantgraph
