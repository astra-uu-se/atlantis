#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElement2dNode : public InvariantNode {
 private:
  size_t _numCols;
  Int _rowOffset;
  Int _colOffset;

 public:
  ArrayVarElement2dNode(IInvariantGraph& graph, VarNodeId rowIndex,
                        VarNodeId colIndex,
                        std::vector<VarNodeId>&& flatVarMatrix,
                        VarNodeId output, size_t numCols, Int rowOffset,
                        Int colOffset);

  ArrayVarElement2dNode(IInvariantGraph& graph, VarNodeId rowIndex,
                        VarNodeId colIndex,
                        const std::vector<std::vector<VarNodeId>>& varMatrix,
                        VarNodeId output, Int rowOffset, Int colOffset);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] size_t flatIndex(Int row, Int col,
                                 bool addOffsets = false) const;

  [[nodiscard]] VarNodeId at(Int row, Int col, bool addOffsets = false) const;

  [[nodiscard]] VarNodeId rowIndex() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId colIndex() const noexcept {
    return staticInputVarNodeIds().back();
  }

  size_t numRows() const noexcept {
    return dynamicInputVarNodeIds().size() / _numCols;
  }

  size_t numCols() const noexcept { return _numCols; }
};

}  // namespace atlantis::invariantgraph
