#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayElement2dNode : public InvariantNode {
 private:
  std::vector<Int> _flatParMatrix;
  size_t _numCols;
  Int _rowOffset;
  Int _colOffset;
  bool _isIntMatrix;

 public:
  ArrayElement2dNode(IInvariantGraph& graph, VarNodeId rowIndex,
                     VarNodeId colIndex,
                     const std::vector<std::vector<Int>>& parMatrix,
                     VarNodeId output, Int rowOffset, Int colOffset);

  ArrayElement2dNode(IInvariantGraph& graph, VarNodeId rowIndex,
                     VarNodeId colIndex, std::vector<Int>&& flatParMatrix,
                     VarNodeId output, size_t numCols, Int rowOffset,
                     Int colOffset);

  ArrayElement2dNode(IInvariantGraph& graph, VarNodeId rowIndex,
                     VarNodeId colIndex,
                     const std::vector<std::vector<bool>>& parMatrix,
                     VarNodeId output, Int rowOffset, Int colOffset);

  ArrayElement2dNode(IInvariantGraph& graph, VarNodeId rowIndex,
                     VarNodeId colIndex, const std::vector<bool>& flatParMatrix,
                     VarNodeId output, size_t numCols, Int rowOffset,
                     Int colOffset);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] size_t flatIndex(Int row, Int col,
                                 bool addOffsets = false) const;

  [[nodiscard]] Int at(Int row, Int col, bool addOffsets = false) const;

  [[nodiscard]] VarNodeId rowIndex() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId colIndex() const noexcept {
    return staticInputVarNodeIds().back();
  }

  [[nodiscard]] size_t numCols() const { return _numCols; }

  [[nodiscard]] size_t numRows() const {
    return _flatParMatrix.size() / _numCols;
  }
};

}  // namespace atlantis::invariantgraph
