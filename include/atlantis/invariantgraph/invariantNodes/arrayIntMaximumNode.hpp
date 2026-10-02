#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMaximumNode : public InvariantNode {
  Int _lowerBound;

 public:
  explicit ArrayIntMaximumNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                               VarNode& output);

  explicit ArrayIntMaximumNode(InvariantGraph& graph,
                               std::vector<std::shared_ptr<VarNode>>&& vars, VarNode& output);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(VarNode& outputVarNodeId) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
