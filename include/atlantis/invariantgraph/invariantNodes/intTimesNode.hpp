#pragma once

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/propagation/invariants/times.hpp"

namespace atlantis::invariantgraph {

class IntTimesNode : public InvariantNode {
 public:
  IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output);

  ~IntTimesNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph
