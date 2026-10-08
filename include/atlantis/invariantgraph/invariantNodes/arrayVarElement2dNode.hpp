#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElement2dNode : public InvariantNode {
  size_t _numRows;
  Int _rowOffset;
  Int _colOffset;

 public:
  ArrayVarElement2dNode(InvariantGraph& graph, VarNode& rowIdx, VarNode& colIdx,
                        std::vector<std::shared_ptr<VarNode>>&& flatVarMatrix,
                        VarNode& output, size_t numRows, Int rowOffset,
                        Int colOffset);

  ArrayVarElement2dNode(
      InvariantGraph& graph, VarNode& rowIdx, VarNode& colIdx,
      std::vector<std::vector<std::shared_ptr<VarNode>>>&& varMatrix,
      VarNode& output, Int rowOffset, Int colOffset);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  bool constrainsOutput(const VarNode& outputVarNode) const override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNode& at(Int row, Int col, bool useOffset = true) const;

  [[nodiscard]] size_t index(Int row, Int col, bool useOffset = true) const;

  [[nodiscard]] VarNode& rowIdx() const noexcept {
    return *staticInputVarNodes().front();
  }

  [[nodiscard]] VarNode& colIdx() const noexcept {
    return *staticInputVarNodes().back();
  }

  [[nodiscard]] size_t numCols() const noexcept {
    return dynamicInputVarNodes().size() / _numRows;
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
