#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayElementNode : public InvariantNode {
  std::vector<Int> _parVector;
  Int _offset;

 public:
  explicit ArrayElementNode(InvariantGraph& graph, std::vector<Int>&& parVector,
                            VarNode& idx, VarNode& output, Int offset);

  explicit ArrayElementNode(InvariantGraph& graph,
                            std::vector<bool>&& parVector, VarNode& idx,
                            VarNode& output, Int offset);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(VarNode& outputVarNodeId) const override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept {
    return _parVector;
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
