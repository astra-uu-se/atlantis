#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntDivNode : public InvariantNode {
 public:
  IntDivNode(InvariantGraph& graph,

             VarNodeId numerator, VarNodeId denominator, VarNodeId quotient);

  void init(const InvariantNodeId&) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] VarNodeId numerator() const noexcept;
  [[nodiscard]] VarNodeId denominator() const noexcept;
  [[nodiscard]] VarNodeId quotient() const noexcept;
};

}  // namespace atlantis::invariantgraph
