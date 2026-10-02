#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntPlusNode : public InvariantNode {
  Int _offset{0};

 public:
  IntPlusNode(InvariantGraph& graph, VarNode& a, VarNode& b, VarNode& output);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      VarNode& outputVarNode) const override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  [[nodiscard]] VarNode& a() const noexcept {
    return *staticInputVarNodes().front();
  }
  [[nodiscard]] VarNode& b() const noexcept {
    return *staticInputVarNodes().back();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
