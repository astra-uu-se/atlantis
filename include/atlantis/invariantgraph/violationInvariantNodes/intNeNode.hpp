#pragma once

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntNeNode : public ViolationInvariantNode {
 public:
  IntNeNode(VarNodeId a, VarNodeId b, VarNodeId r);

  IntNeNode(VarNodeId a, VarNodeId b, bool shouldHold = true);

  bool canBeReplaced(const InvariantGraph& invariantGraph) const override;

  bool replace(InvariantGraph& invariantGraph) override;

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
