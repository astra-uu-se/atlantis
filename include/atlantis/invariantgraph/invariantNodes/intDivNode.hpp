#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntDivNode : public InvariantNode {
  [[nodiscard]] bool updateNumerator();
  [[nodiscard]] bool updateDenominator();
  [[nodiscard]] bool updateQuotient();

 public:
  IntDivNode(InvariantGraph& graph,

             VarNodeId numerator, VarNodeId denominator, VarNodeId quotient);

  void init(InvariantNodeId) override;

  void registerOutputVars(propagation::SolverBase&, SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] VarNodeId numerator() const noexcept;
  [[nodiscard]] VarNodeId denominator() const noexcept;
  [[nodiscard]] VarNodeId quotient() const noexcept;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
