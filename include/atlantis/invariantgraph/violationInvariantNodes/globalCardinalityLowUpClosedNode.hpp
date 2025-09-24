#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityLowUpClosedNode : public ViolationInvariantNode {
  std::vector<VarNodeId> _inputs;
  std::vector<Int> _cover;
  std::vector<Int> _low;
  std::vector<Int> _up;


 public:
  explicit GlobalCardinalityLowUpClosedNode(InvariantGraph& graph,
                                            std::vector<VarNodeId>&& x,
                                            std::vector<Int>&& cover,
                                            std::vector<Int>&& low,
                                            std::vector<Int>&& up, VarNodeId r);

  explicit GlobalCardinalityLowUpClosedNode(InvariantGraph& graph,
                                            std::vector<VarNodeId>&& x,
                                            std::vector<Int>&& cover,
                                            std::vector<Int>&& low,
                                            std::vector<Int>&& up,
                                            bool shouldHold = true);

  void init(InvariantNodeId) override;
  void updateState();

  void registerOutputVars(propagation::SolverBase&, SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
