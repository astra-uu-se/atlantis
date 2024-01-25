#pragma once

#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/boolOr.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

class BoolOrNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  BoolOrNode(VarNodeId a, VarNodeId b, VarNodeId r);

  BoolOrNode(VarNodeId a, VarNodeId b, bool shouldHold);

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