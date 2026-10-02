#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntModViewNode : public InvariantNode {
  Int _denominator;

 public:
  IntModViewNode(InvariantGraph& graph, VarNode& staticInput, VarNode& output,
                 Int denominator);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      const VarNode& outputVarNode) const override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNode& input() const noexcept {
    return staticInputVarNodes().front();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
