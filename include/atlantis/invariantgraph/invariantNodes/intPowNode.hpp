#pragma once

#include <cmath>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/pow.hpp"

namespace atlantis::invariantgraph {

class IntPowNode : public InvariantNode {
 public:
  IntPowNode(VarNodeId base, VarNodeId exponent, VarNodeId power);

  ~IntPowNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId base() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId exponent() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph