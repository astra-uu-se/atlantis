#pragma once

#include <utility>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllDifferentNode(std::vector<VarNodeId>&& vars, bool shouldHold);

  bool prune(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
