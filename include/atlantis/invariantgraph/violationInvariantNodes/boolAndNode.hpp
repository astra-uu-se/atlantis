#pragma once

#include <utility>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/invariants/boolAnd.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

class BoolAndNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  BoolAndNode(VarNodeId a, VarNodeId b, VarNodeId r);

  BoolAndNode(VarNodeId a, VarNodeId b, bool shouldHold);

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
