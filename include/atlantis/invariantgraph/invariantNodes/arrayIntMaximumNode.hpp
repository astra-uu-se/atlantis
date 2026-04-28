#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMaximumNode : public InvariantNode {
  Int _lb;

 public:
  explicit ArrayIntMaximumNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                               VarNodeId output);

  explicit ArrayIntMaximumNode(InvariantGraph& graph,

                               std::vector<VarNodeId>&& vars, VarNodeId output);

  void init(InvariantNodeId) override;
  void postConstraint();

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
