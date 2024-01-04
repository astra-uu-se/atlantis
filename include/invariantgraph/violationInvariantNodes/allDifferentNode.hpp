#pragma once

#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/views/notEqualConst.hpp"
#include "propagation/violationInvariants/allDifferent.hpp"
#include "propagation/violationInvariants/int_ne.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllDifferentNode(std::vector<VarNodeId>&& vars, bool shouldHold);

  bool canBeReplaced() const override;

  void replace(InvariantGraph&) override;

  bool canBeRemoved() const override;

  void remove() override;

  bool prune(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph