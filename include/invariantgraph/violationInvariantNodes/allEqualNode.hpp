#pragma once

#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/views/equalConst.hpp"
#include "propagation/views/notEqualConst.hpp"
#include "propagation/violationInvariants/allDifferent.hpp"
#include "propagation/violationInvariants/intEqNode.hpp"

namespace atlantis::invariantgraph {

class AllEqualNode : public ViolationInvariantNode {
 private:
  propagation::VarId _allDifferentViolationVarId{propagation::NULL_ID};

 public:
  explicit AllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllEqualNode(std::vector<VarNodeId>&& vars, bool shouldHold = true);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph