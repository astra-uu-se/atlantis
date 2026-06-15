#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
  std::vector<Int> _seenValues;

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

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
