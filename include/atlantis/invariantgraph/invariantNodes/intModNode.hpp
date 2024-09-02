#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntModNode : public InvariantNode {
 public:
  IntModNode(InvariantGraph& graph,

             VarNodeId numerator, VarNodeId denominator, VarNodeId remainder);

  void init(const InvariantNodeId&) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId numerator() const;
  [[nodiscard]] VarNodeId denominator() const;
  [[nodiscard]] VarNodeId remainder() const;
};

}  // namespace atlantis::invariantgraph
