#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityLowUpNode : public ViolationInvariantNode {
  std::vector<VarNodeId> _inputs;
  std::vector<Int> _cover;
  std::vector<Int> _low;
  std::vector<Int> _up;

 public:
  explicit GlobalCardinalityLowUpNode(InvariantGraph& graph,
                                      std::vector<VarNodeId>&& x,
                                      std::vector<Int>&& cover,
                                      std::vector<Int>&& low,
                                      std::vector<Int>&& up, VarNodeId r);

  explicit GlobalCardinalityLowUpNode(InvariantGraph& graph,
                                      std::vector<VarNodeId>&& x,
                                      std::vector<Int>&& cover,
                                      std::vector<Int>&& low,
                                      std::vector<Int>&& up,
                                      bool shouldHold = true);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
