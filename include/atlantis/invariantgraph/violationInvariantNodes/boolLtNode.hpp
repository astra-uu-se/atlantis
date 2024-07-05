#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class BoolLtNode : public ViolationInvariantNode {
 public:
  BoolLtNode(VarNodeId a, VarNodeId b, VarNodeId r);

  BoolLtNode(VarNodeId a, VarNodeId b, bool shouldHold = true);

  void updateState(InvariantGraph& graph) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph&) override;

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
