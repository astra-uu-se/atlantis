#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class IntAllEqualNode : public ViolationInvariantNode {
 private:
  bool _breaksCycle{false};
  propagation::VarId _allDifferentViolationVarId{propagation::NULL_ID};

 public:
  explicit IntAllEqualNode(VarNodeId a, VarNodeId b, VarNodeId r,
                           bool breaksCycle = false);

  explicit IntAllEqualNode(VarNodeId a, VarNodeId b, bool shouldHold = true,
                           bool breaksCycle = false);

  explicit IntAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r,
                           bool breaksCycle = false);

  explicit IntAllEqualNode(std::vector<VarNodeId>&& vars,
                           bool shouldHold = true, bool breaksCycle = false);

  bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph& graph) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
