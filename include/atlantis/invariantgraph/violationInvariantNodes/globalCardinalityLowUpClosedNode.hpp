#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityLowUpClosedNode : public ViolationInvariantNode {
 private:
  std::vector<VarNodeId> _inputs;
  std::vector<Int> _cover;
  std::vector<Int> _low;
  std::vector<Int> _up;
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  explicit GlobalCardinalityLowUpClosedNode(IInvariantGraph& graph,
                                            std::vector<VarNodeId>&& x,
                                            std::vector<Int>&& cover,
                                            std::vector<Int>&& low,
                                            std::vector<Int>&& up, VarNodeId r);

  explicit GlobalCardinalityLowUpClosedNode(IInvariantGraph& graph,
                                            std::vector<VarNodeId>&& x,
                                            std::vector<Int>&& cover,
                                            std::vector<Int>&& low,
                                            std::vector<Int>&& up,
                                            bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;
};
}  // namespace atlantis::invariantgraph
