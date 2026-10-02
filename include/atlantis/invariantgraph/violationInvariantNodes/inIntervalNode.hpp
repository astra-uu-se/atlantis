#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class InIntervalNode : public ViolationInvariantNode {
  Int _lb, _ub;

 public:
  explicit InIntervalNode(InvariantGraph& graph, VarNode& input, Int lb, Int ub,
                          VarNode& r);

  explicit InIntervalNode(InvariantGraph& graph, VarNode& input, Int lb, Int ub,
                          bool shouldHold = true);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
