#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

class ArrayElementNode : public InvariantNode {
 private:
  std::vector<Int> _parVector;
  Int _offset;
  bool _isIntVector;

 public:
  ArrayElementNode(InvariantGraph& graph, std::vector<Int>&& parVector,
                   VarNodeId idx, VarNodeId output, Int offset,
                   bool isIntVector = true);

  ArrayElementNode(InvariantGraph& graph, std::vector<bool>&& parVector,
                   VarNodeId idx, VarNodeId output, Int offset);

  void init(const InvariantNodeId&) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept {
    return _parVector;
  }
  [[nodiscard]] VarNodeId idx() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph
