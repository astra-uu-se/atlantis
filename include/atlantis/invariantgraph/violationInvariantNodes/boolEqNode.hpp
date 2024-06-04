#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class BoolEqNode : public ViolationInvariantNode {
 private:
  bool _breaksCycle{false};

 public:
  explicit BoolEqNode(VarNodeId a, VarNodeId b, VarNodeId r,
                      bool breaksCycle = false);

  explicit BoolEqNode(VarNodeId a, VarNodeId b, bool shouldHold = true,
                      bool breaksCycle = false);

  bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph& graph) override;

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
