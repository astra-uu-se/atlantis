#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntPowNode : public InvariantNode {
 public:
  IntPowNode(InvariantGraph& graph, VarNodeId base, VarNodeId exponent,
             VarNodeId power);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId base() const;
  [[nodiscard]] VarNodeId exponent() const;
  [[nodiscard]] VarNodeId power() const;
};

}  // namespace atlantis::invariantgraph
