#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class CountRelNode : public ViolationInvariantNode {
  std::optional<Int> _fixedNeedle;
  std::optional<Int> _fixedAmount;
  Int _offset{0};
  RelationType _relType;

  [[nodiscard]] VarNodeId needle() const;
  [[nodiscard]] size_t needleIndex() const;
  [[nodiscard]] VarNodeId amount() const;
  [[nodiscard]] size_t amountIndex() const;
  [[nodiscard]] size_t numInputVars() const;

 public:
  CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars, Int needle,
               Int amount, RelationType relationType, bool shouldHold = true);

  CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars, Int needle,
               Int amount, RelationType relationType, VarNodeId reified);

  CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars, Int needle,
               VarNodeId amount, RelationType relationType,
               bool shouldHold = true);

  CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars, Int needle,
               VarNodeId amount, RelationType relationType, VarNodeId reified);

  CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
               VarNodeId needle, Int amount, RelationType relationType,
               bool shouldHold = true);

  CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
               VarNodeId needle, Int amount, RelationType relationType,
               VarNodeId reified);

  CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
               VarNodeId needle, VarNodeId amount, RelationType relationType,
               bool shouldHold = true);

  CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
               VarNodeId needle, VarNodeId amount, RelationType relationType,
               VarNodeId reified);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
