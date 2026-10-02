#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityLowUpClosedNode : public ViolationInvariantNode {
  std::vector<std::shared_ptr<VarNode>> _inputs;
  std::vector<Int> _cover;
  std::vector<Int> _low;
  std::vector<Int> _up;

 public:
  explicit GlobalCardinalityLowUpClosedNode(
      InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& x,
      std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up,
      VarNode& r);

  explicit GlobalCardinalityLowUpClosedNode(
      InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& x,
      std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up,
      bool shouldHold = true);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
