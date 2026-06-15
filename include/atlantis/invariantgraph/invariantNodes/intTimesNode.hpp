#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntTimesNode : public InvariantNode {
  std::optional<Int> _scalar;

 public:
  IntTimesNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
               VarNodeId output);

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
