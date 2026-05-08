#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElement2dNode : public InvariantNode {
  size_t _numRows;
  Int _rowOffset;
  Int _colOffset;

 public:
  ArrayVarElement2dNode(InvariantGraph& graph,

                        VarNodeId rowIdx, VarNodeId colIdx,
                        std::vector<VarNodeId>&& flatVarMatrix,
                        VarNodeId output, size_t numRows, Int rowOffset,
                        Int colOffset);

  ArrayVarElement2dNode(InvariantGraph& graph,

                        VarNodeId rowIdx, VarNodeId colIdx,
                        std::vector<std::vector<VarNodeId>>&& varMatrix,
                        VarNodeId output, Int rowOffset, Int colOffset);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] VarNodeId at(Int row, Int col, bool useOffset = true) const;

  [[nodiscard]] size_t index(Int row, Int col, bool useOffset = true) const;

  [[nodiscard]] VarNodeId rowIdx() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId colIdx() const noexcept {
    return staticInputVarNodeIds().back();
  }

  [[nodiscard]] size_t numCols() const noexcept {
    return dynamicInputVarNodeIds().size() / _numRows;
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
