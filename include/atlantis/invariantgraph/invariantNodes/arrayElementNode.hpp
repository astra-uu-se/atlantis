#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayElementNode : public InvariantNode {
 private:
  std::vector<Int> _parVector;
  Int _offset;
  bool _isIntVector;

 public:
  ArrayElementNode(IInvariantGraph& graph, std::vector<Int>&& parVector,
                   VarNodeId idx, VarNodeId output, Int offset,
                   bool isIntVector = true);

  ArrayElementNode(IInvariantGraph& graph, std::vector<bool>&& parVector,
                   VarNodeId idx, VarNodeId output, Int offset);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept {
    return _parVector;
  }
  [[nodiscard]] VarNodeId idx() const noexcept {
    return staticInputVarNodeIds().back();
  }

  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
