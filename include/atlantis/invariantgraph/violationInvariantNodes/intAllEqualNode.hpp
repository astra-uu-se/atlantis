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
  propagation::VarId _allDifferentViolationVarId{propagation::NULL_ID};

 public:
  explicit IntAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit IntAllEqualNode(std::vector<VarNodeId>&& vars,
                           bool shouldHold = true);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
