#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayElement2dNode : public InvariantNode {
  std::vector<std::vector<Int>> _parMatrix;
  Int _rowOffset;
  Int _colOffset;
  bool _isIntMatrix;

 public:
  ArrayElement2dNode(InvariantGraph& graph, VarNode& rowIdx, VarNode& colIdx,
                     std::vector<std::vector<Int>>&& parMatrix,
                     VarNode& output, Int rowOffset, Int colOffset,
                     bool isIntMatrix = true);

  ArrayElement2dNode(InvariantGraph& graph, VarNode& rowIdx, VarNode& colIdx,
                     const std::vector<std::vector<bool>>& parMatrix,
                     VarNode& output, Int rowOffset, Int colOffset);

  void postConstraint() override;

  void init() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(const VarNode& outputVarNode) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const VarNode& rowIdx() const noexcept {
    return *staticInputVarNodes().front();
  }

  [[nodiscard]] const VarNode& colIdx() const noexcept {
    return *staticInputVarNodes().back();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
