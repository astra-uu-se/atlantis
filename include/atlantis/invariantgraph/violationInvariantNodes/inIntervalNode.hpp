#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class InIntervalNode : public ViolationInvariantNode {
 private:
  Int _lb, _ub;
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  explicit InIntervalNode(IInvariantGraph& graph, VarNodeId input, Int lb,
                          Int ub, VarNodeId r);

  explicit InIntervalNode(IInvariantGraph& graph, VarNodeId input, Int lb,
                          Int ub, bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;
  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
