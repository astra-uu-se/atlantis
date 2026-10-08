#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntAbsNode : public InvariantNode {
 public:
  IntAbsNode(InvariantGraph& graph, VarNode& staticInput, VarNode& output);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      const VarNode& outputVarNode) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNode& input() const noexcept {
    return *staticInputVarNodes().front();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
