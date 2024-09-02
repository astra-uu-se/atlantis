#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayElement2dNode : public InvariantNode {
 private:
  std::vector<std::vector<Int>> _parMatrix;
  Int _offset1;
  Int _offset2;
  bool _isIntMatrix;

 public:
  ArrayElement2dNode(IInvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
                     std::vector<std::vector<Int>>&& parMatrix,
                     VarNodeId output, Int offset1, Int offset2);

  ArrayElement2dNode(IInvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
                     std::vector<std::vector<bool>>&& parMatrix,
                     VarNodeId output, Int offset1, Int offset2);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId idx1() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId idx2() const noexcept {
    return staticInputVarNodeIds().back();
  }

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
