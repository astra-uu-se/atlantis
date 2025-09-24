#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElementNode : public InvariantNode {
  Int _offset;

 public:
  ArrayVarElementNode(InvariantGraph& graph, VarNodeId idx,
                      std::vector<VarNodeId>&& varVector, VarNodeId output,
                      Int offset);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&, SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNodeId idx() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
