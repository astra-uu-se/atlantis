#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
 private:
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  explicit AllDifferentNode(IInvariantGraph& graph,

                            VarNodeId a, VarNodeId b, VarNodeId r);

  explicit AllDifferentNode(IInvariantGraph& graph,

                            VarNodeId a, VarNodeId b, bool shouldHold = true);

  explicit AllDifferentNode(IInvariantGraph& graph,

                            std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllDifferentNode(IInvariantGraph& graph,

                            std::vector<VarNodeId>&& vars,
                            bool shouldHold = true);

  void init(InvariantNodeId) override;

  virtual void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  void registerOutputVars() override;

  void registerNode() override;
  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
