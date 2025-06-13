#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  explicit AllDifferentNode(InvariantGraph& graph,

                            VarNodeId a, VarNodeId b, VarNodeId r);

  explicit AllDifferentNode(InvariantGraph& graph,

                            VarNodeId a, VarNodeId b, bool shouldHold = true);

  explicit AllDifferentNode(InvariantGraph& graph,

                            std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllDifferentNode(InvariantGraph& graph,

                            std::vector<VarNodeId>&& vars,
                            bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
