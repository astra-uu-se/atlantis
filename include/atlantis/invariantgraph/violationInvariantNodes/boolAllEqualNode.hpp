#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class BoolAllEqualNode : public ViolationInvariantNode {
 private:
  bool _breaksCycle{false};
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  explicit BoolAllEqualNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                            VarNodeId r, bool breaksCycle = false);

  explicit BoolAllEqualNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                            bool shouldHold = true, bool breaksCycle = false);

  explicit BoolAllEqualNode(IInvariantGraph& graph,
                            std::vector<VarNodeId>&& vars, VarNodeId r,
                            bool breaksCycle = false);

  explicit BoolAllEqualNode(IInvariantGraph& graph,
                            std::vector<VarNodeId>&& vars,
                            bool shouldHold = true, bool breaksCycle = false);

  void init(InvariantNodeId) override;

  void updateState() override;

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
