#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElement2dNode : public InvariantNode {
  size_t _numRows;
  Int _offset1;
  Int _offset2;

 public:
  ArrayVarElement2dNode(InvariantGraph& graph,

                        VarNodeId idx1, VarNodeId idx2,
                        std::vector<VarNodeId>&& flatVarMatrix,
                        VarNodeId output, size_t numRows, Int offset1,
                        Int offset2);

  ArrayVarElement2dNode(InvariantGraph& graph,

                        VarNodeId idx1, VarNodeId idx2,
                        std::vector<std::vector<VarNodeId>>&& varMatrix,
                        VarNodeId output, Int offset1, Int offset2);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId at(Int row, Int col) const;

  [[nodiscard]] VarNodeId idx1() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId idx2() const noexcept {
    return staticInputVarNodeIds().back();
  }

  [[nodiscard]] size_t numCols() const noexcept {
    return dynamicInputVarNodeIds().size() / _numRows;
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
