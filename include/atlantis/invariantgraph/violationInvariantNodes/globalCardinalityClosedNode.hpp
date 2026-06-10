#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityClosedNode : public ViolationInvariantNode {
  std::vector<Int> _cover;
  std::vector<Int> _offsets;

 public:
  explicit GlobalCardinalityClosedNode(InvariantGraph& graph,
                                       std::vector<VarNodeId>&& inputs,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       VarNodeId r);

  explicit GlobalCardinalityClosedNode(InvariantGraph& graph,
                                       std::vector<VarNodeId>&& inputs,
                                       std::vector<Int>&& cover,
                                       std::vector<VarNodeId>&& counts,
                                       bool shouldHold = true);

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
