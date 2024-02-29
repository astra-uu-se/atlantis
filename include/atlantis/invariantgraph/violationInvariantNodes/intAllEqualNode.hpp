#pragma once

#include <utility>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"

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
