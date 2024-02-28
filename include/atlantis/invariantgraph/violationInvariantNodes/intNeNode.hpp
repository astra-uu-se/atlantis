#pragma once

#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/violationInvariants/equal.hpp"
#include "propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

class IntNeNode : public ViolationInvariantNode {
 public:
  IntNeNode(VarNodeId a, VarNodeId b, VarNodeId r);

  IntNeNode(VarNodeId a, VarNodeId b, bool shouldHold);

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