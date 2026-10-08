#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation solver.
 */
class ViolationInvariantNode : public InvariantNode {
  bool _isReified;
  bool _shouldHold;

  void updateReified();

  explicit ViolationInvariantNode(InvariantGraph& graph,
                                  std::vector<std::shared_ptr<VarNode>>&& outputs,
                                  std::vector<std::shared_ptr<VarNode>>&& inputs,
                                  const std::shared_ptr<VarNode>& reifiedViolation,
                                  bool shouldHold);

 protected:
  propagation::VarViewId setViolationVarId(propagation::VarViewId,
                                           SolverMapping&) const;

  propagation::VarViewId registerViolation(Int initialValue,
                                           propagation::SolverBase&,
                                           SolverMapping&) const;

  propagation::VarViewId registerViolation(propagation::SolverBase&,
                                           SolverMapping&) const;

  [[nodiscard]] bool shouldHold() const noexcept;

  void setShouldHold(bool sh) noexcept;

  void fixReified(bool);

 public:
  explicit ViolationInvariantNode(InvariantGraph& graph,
                                  std::vector<std::shared_ptr<VarNode>>&& outputs,
                                  std::vector<std::shared_ptr<VarNode>>&& staticInputs,
                                  VarNode& reifiedViolation);

  explicit ViolationInvariantNode(InvariantGraph& graph,
                                  std::vector<std::shared_ptr<VarNode>>&& staticInputs,
                                  VarNode& reifiedViolation);

  explicit ViolationInvariantNode(InvariantGraph& graph,
                                  std::vector<std::shared_ptr<VarNode>>&& outputs,
                                  std::vector<std::shared_ptr<VarNode>>&& staticInputs,
                                  bool shouldHold);

  explicit ViolationInvariantNode(InvariantGraph& graph,
                                  std::vector<std::shared_ptr<VarNode>>&& staticInputs,
                                  bool shouldHold);

  void init() override;

  [[nodiscard]] bool isReified() const override;

  [[nodiscard]] bool isViolationInvariant() const override;

  [[nodiscard]] propagation::VarViewId violationVarId(
      const SolverMapping&) const override;

  [[nodiscard]] std::shared_ptr<VarNode> reifiedViolationNode();

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(const VarNode&) const override;
};

}  // namespace atlantis::invariantgraph
