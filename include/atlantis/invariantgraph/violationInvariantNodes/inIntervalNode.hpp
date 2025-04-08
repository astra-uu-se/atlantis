#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class InIntervalNode : public ViolationInvariantNode {
  Int _lb, _ub;
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  explicit InIntervalNode(InvariantGraph& graph, VarNodeId input, Int lb,
                          Int ub, VarNodeId r);

  explicit InIntervalNode(InvariantGraph& graph, VarNodeId input, Int lb,
                          Int ub, bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
