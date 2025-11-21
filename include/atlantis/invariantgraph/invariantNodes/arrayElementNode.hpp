#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayElementNode : public InvariantNode {
  std::vector<Int> _parVector;
  Int _offset;
  bool _isIntVector;

 public:
  ArrayElementNode(InvariantGraph& graph, std::vector<Int>&& parVector,
                   VarNodeId idx, VarNodeId output, Int offset,
                   bool isIntVector = true);

  ArrayElementNode(InvariantGraph& graph, std::vector<bool>&& parVector,
                   VarNodeId idx, VarNodeId output, Int offset);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept {
    return _parVector;
  }
  [[nodiscard]] VarNodeId idx() const noexcept {
    return staticInputVarNodeIds().back();
  }

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
