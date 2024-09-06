#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntScalarNode : public InvariantNode {
 private:
  Int _factor;
  Int _offset;

 public:
  IntScalarNode(IInvariantGraph& graph, VarNodeId staticInput, VarNodeId output,
                Int factor, Int offset);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId input() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace atlantis::invariantgraph
