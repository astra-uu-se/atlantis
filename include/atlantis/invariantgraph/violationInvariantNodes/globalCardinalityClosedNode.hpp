#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityClosedNode : public ViolationInvariantNode {
  std::vector<Int> _cover;
  std::vector<Int> _countOffsets;

 public:
  explicit GlobalCardinalityClosedNode(
      InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputs,
      std::vector<Int>&& cover, std::vector<std::shared_ptr<VarNode>>&& counts,
      VarNode& r);

  explicit GlobalCardinalityClosedNode(
      InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputs,
      std::vector<Int>&& cover, std::vector<std::shared_ptr<VarNode>>&& counts,
      bool shouldHold = true);

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void init() override;

  void postConstraint() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
