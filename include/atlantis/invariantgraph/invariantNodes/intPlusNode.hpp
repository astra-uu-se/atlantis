#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntPlusNode : public InvariantNode {
  Int _offset{0};

 public:
  IntPlusNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
              VarNodeId output);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
