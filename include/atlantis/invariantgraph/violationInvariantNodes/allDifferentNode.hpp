#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
  std::vector<Int> _seenValues;

 public:
  explicit AllDifferentNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                            VarNode& r);

  explicit AllDifferentNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                            bool shouldHold = true);

  explicit AllDifferentNode(InvariantGraph& graph,
                            std::vector<std::shared_ptr<VarNode>>&& vars,
                            VarNode& r);

  explicit AllDifferentNode(InvariantGraph& graph,
                            std::vector<std::shared_ptr<VarNode>>&& vars,
                            bool shouldHold = true);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] std::pair<size_t, size_t> implicitRank() const override;

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
