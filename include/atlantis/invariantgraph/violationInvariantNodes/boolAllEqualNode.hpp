#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class BoolAllEqualNode : public ViolationInvariantNode {
 private:
  bool _breaksCycle{false};
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit BoolAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r,
                            bool breaksCycle = false);

  explicit BoolAllEqualNode(std::vector<VarNodeId>&& vars,
                            bool shouldHold = true, bool breaksCycle = false);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
