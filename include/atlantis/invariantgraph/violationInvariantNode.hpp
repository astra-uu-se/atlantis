#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation solver.
 */
class ViolationInvariantNode : public InvariantNode {
 private:
  // Bounds will be recomputed by the solver.
  propagation::VarId _violationVarId{propagation::NULL_ID};
  bool _isReified;

  // If the violation invariant is not reified, then this boolean indicates if
  // the violation invariant should hold or not:
  bool _shouldHold;

  void updateReified();

  explicit ViolationInvariantNode(IInvariantGraph& graph,
                                  std::vector<VarNodeId>&& outputIds,
                                  std::vector<VarNodeId>&& staticInputIds,
                                  VarNodeId reifiedViolationId,
                                  bool shouldHold);

 protected:
  propagation::VarId setViolationVarId(propagation::VarId);

  propagation::VarId registerViolation(Int initialValue = 0);

  [[nodiscard]] bool shouldHold() const noexcept;

  void fixReified(bool);

 public:
  explicit ViolationInvariantNode(IInvariantGraph& graph,
                                  std::vector<VarNodeId>&& outputIds,
                                  std::vector<VarNodeId>&& staticInputIds,
                                  VarNodeId reifiedViolationId);

  explicit ViolationInvariantNode(IInvariantGraph& graph,
                                  std::vector<VarNodeId>&& staticInputIds,
                                  VarNodeId reifiedViolationId);

  explicit ViolationInvariantNode(IInvariantGraph& graph,
                                  std::vector<VarNodeId>&& outputIds,
                                  std::vector<VarNodeId>&& staticInputIds,
                                  bool shouldHold);

  explicit ViolationInvariantNode(IInvariantGraph& graph,
                                  std::vector<VarNodeId>&& staticInputIds,
                                  bool shouldHold);

  void init(InvariantNodeId) override;

  [[nodiscard]] bool isReified() const override;

  [[nodiscard]] propagation::VarId violationVarId() const override;

  VarNodeId reifiedViolationNodeId();

  virtual void updateState() override;
};

}  // namespace atlantis::invariantgraph
