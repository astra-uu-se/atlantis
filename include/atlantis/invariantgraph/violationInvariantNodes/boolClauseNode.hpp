#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class BoolClauseNode : public ViolationInvariantNode {
  size_t _numPosVars;
  propagation::VarViewId _sumVarId{propagation::NULL_ID};

 public:
  explicit BoolClauseNode(InvariantGraph& graph,
                          std::vector<VarNodeId>&& posVars,
                          std::vector<VarNodeId>&& negVars, VarNodeId r);

  explicit BoolClauseNode(InvariantGraph& graph,
                          std::vector<VarNodeId>&& posVars,
                          std::vector<VarNodeId>&& negVars,
                          bool shouldHold = true);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
