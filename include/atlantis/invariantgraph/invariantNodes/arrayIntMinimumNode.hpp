#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMinimumNode : public InvariantNode {
  Int _upperBound;

 public:
  explicit ArrayIntMinimumNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                               VarNode& output);

  explicit ArrayIntMinimumNode(InvariantGraph& graph,
                               std::vector<std::shared_ptr<VarNode>>&& vars,
                               VarNode& output);

  void init() override;

  void postConstraint() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      const VarNode& outputVarNode) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
