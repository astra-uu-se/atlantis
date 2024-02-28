#pragma once

#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/violationInvariants/boolLessEqual.hpp"
#include "propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {

class BoolLeNode : public ViolationInvariantNode {
 public:
  BoolLeNode(VarNodeId a, VarNodeId b, VarNodeId r);

  BoolLeNode(VarNodeId a, VarNodeId b, bool shouldHold);

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