#include "invariantgraph/violationInvariantNode.hpp"

#include "invariantgraph/invariantGraph.hpp"

namespace invariantgraph {

static std::vector<invariantgraph::VarNodeId> combine(
    VarNodeId reifiedViolation, std::vector<VarNodeId>&& definedVars) {
  if (reifiedViolation == nullptr) {
    return std::move(definedVars);
  }
  definedVars.insert(definedVars.begin(), reifiedViolation);
  return std::move(definedVars);
}

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation engine.
 */

explicit ViolationInvariantNode::ViolationInvariantNode(
    InvariantNodeId id, std::vector<VarNodeId>&& definedVars,
    std::vector<VarNodeId>&& staticInputVarNodeIds, VarNodeId reifiedViolation,
    bool shouldHold)
    : InvariantNode(
          id, std::move(combine(reifiedViolation, std::move(definedVars))),
          std::move(staticInputVarNodeIds)),
      _reifiedViolation(reifiedViolation),
      _shouldHold(shouldHold) {
  if (!isReified()) {
    assert(_reifiedViolation == nullptr);
  } else {
    assert(_reifiedViolation != nullptr);
    assert(_reifiedViolation->definingNodes().contains(this));
    assert(outputVarNodeIds().front() == _reifiedViolation);
  }
}

inline bool ViolationInvariantNode::shouldHold() const noexcept {
  return _shouldHold;
}

explicit ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& outputVarNodeIds,
    std::vector<VarNodeId>&& staticInputVarNodeIds, VarNodeId reifiedViolation)
    : ViolationInvariantNode(std::move(outputVarNodeIds),
                             std::move(staticInputVarNodeIds), reifiedViolation,
                             true) {}

explicit ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& staticInputVarNodeIds, VarNodeId reifiedViolation)
    : ViolationInvariantNode({}, std::move(staticInputVarNodeIds),
                             reifiedViolation, true) {}

explicit ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& outputVarNodeIds,
    std::vector<VarNodeId>&& staticInputVarNodeIds, bool shouldHold)
    : ViolationInvariantNode(std::move(outputVarNodeIds),
                             std::move(staticInputVarNodeIds), nullptr,
                             shouldHold) {}

explicit ViolationInvariantNode::ViolationInvariantNode(
    std::vector<VarNodeId>&& staticInputVarNodeIds, bool shouldHold)
    : ViolationInvariantNode({}, std::move(staticInputVarNodeIds), nullptr,
                             shouldHold) {}
bool ViolationInvariantNode::isReified() const override {
  return _reifiedViolation != nullptr;
}

VarId ViolationInvariantNode::violationVarId() const override {
  if (isReified()) {
    return _reifiedViolation->varId();
  }
  return _violationVarId;
}

inline VarNodeId ViolationInvariantNode::reifiedViolation() {
  return _reifiedViolation;
}

VarId ViolationInvariantNode::setViolationVarId(VarId varId) {
  assert(violationVarId() == NULL_ID);
  if (isReified()) {
    _reifiedViolation->setVarId(varId);
  } else {
    _violationVarId = varId;
  }
  return violationVarId();
}

inline VarId ViolationInvariantNode::registerViolation(Engine& engine,
                                                       Int initialValue) {
  if (violationVarId() == NULL_ID) {
    return setViolationVarId(
        engine.makeIntVar(initialValue, initialValue, initialValue));
  }
  return violationVarId();
}
};

}  // namespace invariantgraph