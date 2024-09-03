#pragma once

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class Int2BoolNode : public InvariantNode {
 public:
  Int2BoolNode(InvariantGraph& graph, VarNodeId staticInput, VarNodeId output);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId input() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace atlantis::invariantgraph
