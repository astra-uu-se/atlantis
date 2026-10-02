#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElementNode : public InvariantNode {
  Int _offset;

 public:
  ArrayVarElementNode(InvariantGraph& graph, VarNode& idx,
                      std::vector<std::shared_ptr<VarNode>>&& varVector,
                      VarNode& output, Int offset);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      VarNode& outputVarNode) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNode& idx() const noexcept {
    return *staticInputVarNodes().front();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
