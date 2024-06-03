#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntPowNode : public InvariantNode {
 public:
  IntPowNode(VarNodeId base, VarNodeId exponent, VarNodeId power);

  ~IntPowNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  void propagate(InvariantGraph& graph) override;

  [[nodiscard]] VarNodeId base() const;
  [[nodiscard]] VarNodeId exponent() const;
  [[nodiscard]] VarNodeId power() const;
};

}  // namespace atlantis::invariantgraph
