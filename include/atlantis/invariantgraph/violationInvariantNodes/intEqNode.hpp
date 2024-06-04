#pragma once

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntEqNode : public ViolationInvariantNode {
 private:
  bool _breaksCycle{false};

 public:
  explicit IntEqNode(VarNodeId a, VarNodeId b, VarNodeId r,
                     bool breaksCycles = false);

  explicit IntEqNode(VarNodeId a, VarNodeId b, bool shouldHold = true,
                     bool breaksCycles = false);

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph& graph) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }
  [[nodiscard]] bool breaksCycle() const noexcept { return _breaksCycle; }
};

}  // namespace atlantis::invariantgraph
