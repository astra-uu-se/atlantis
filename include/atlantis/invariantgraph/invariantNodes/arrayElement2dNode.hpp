#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayElement2dNode : public InvariantNode {
  std::vector<std::vector<Int>> _parMatrix;
  Int _rowOffset;
  Int _colOffset;
  bool _isIntMatrix;

 public:
  ArrayElement2dNode(InvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
                     std::vector<std::vector<Int>>&& parMatrix,
                     VarNodeId output, Int offset1, Int offset2);

  ArrayElement2dNode(InvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
                     std::vector<std::vector<bool>>&& parMatrix,
                     VarNodeId output, Int offset1, Int offset2);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId rowIdx() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId colIdx() const noexcept {
    return staticInputVarNodeIds().back();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
