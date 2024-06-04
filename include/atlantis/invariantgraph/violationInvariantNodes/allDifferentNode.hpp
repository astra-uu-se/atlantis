#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r);

  explicit AllDifferentNode(std::vector<VarNodeId>&& vars,
                            bool shouldHold = true);

  virtual void updateVariableNodes(InvariantGraph& graph) override;

  bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph& graph) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
