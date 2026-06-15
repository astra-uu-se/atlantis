#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntPowNode : public InvariantNode {
 public:
  IntPowNode(InvariantGraph& graph, VarNodeId base, VarNodeId exponent,
             VarNodeId power);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNodeId base() const;
  [[nodiscard]] VarNodeId exponent() const;
  [[nodiscard]] VarNodeId power() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
