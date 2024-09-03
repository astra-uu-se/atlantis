#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElementNode : public InvariantNode {
 private:
  Int _offset;

 public:
  ArrayVarElementNode(InvariantGraph& graph, VarNodeId idx,
                      std::vector<VarNodeId>&& varVector, VarNodeId output,
                      Int offset);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId idx() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace atlantis::invariantgraph
