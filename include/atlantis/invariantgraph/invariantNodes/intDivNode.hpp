#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntDivNode : public InvariantNode {
 public:
  IntDivNode(InvariantGraph& graph, VarNode& numerator, VarNode& denominator,
             VarNode& quotient);

  void init() override;

  void postConstraint() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(const VarNode&) const override {
    return true;
  }

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] VarNode& numerator() const noexcept;
  [[nodiscard]] VarNode& denominator() const noexcept;
  [[nodiscard]] VarNode& quotient() const noexcept;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
