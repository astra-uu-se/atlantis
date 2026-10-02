#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntPowNode : public InvariantNode {
 public:
  IntPowNode(InvariantGraph& graph, VarNode& base, VarNode& exponent,
             VarNode& power);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(const VarNode&) const override {
    return true;
  }

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNode& base() const;

  [[nodiscard]] VarNode& exponent() const;

  [[nodiscard]] VarNode& power() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
