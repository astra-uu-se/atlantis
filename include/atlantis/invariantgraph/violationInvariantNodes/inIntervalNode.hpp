#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class InIntervalNode : public ViolationInvariantNode {
  Int _lb, _ub;

 public:
  explicit InIntervalNode(InvariantGraph& graph, VarNodeId input, Int lb,
                          Int ub, VarNodeId r);

  explicit InIntervalNode(InvariantGraph& graph, VarNodeId input, Int lb,
                          Int ub, bool shouldHold = true);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
