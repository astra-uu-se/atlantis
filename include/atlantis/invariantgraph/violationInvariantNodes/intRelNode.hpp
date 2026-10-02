#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class IntRelNode : public ViolationInvariantNode {
  RelationType _relType;
  std::optional<Int> _fixedRhs{std::nullopt};

 public:
  IntRelNode(InvariantGraph& graph, VarNode& a, RelationType, VarNode& b,
             VarNode& r);

  IntRelNode(InvariantGraph& graph, VarNode& a, RelationType, VarNode& b,
             bool shouldHold = true);

  void init() override;

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
