#pragma once

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntTimesNode : public InvariantNode {
 private:
  Int _scalar{1};

 public:
  IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output);

  void updateState(InvariantGraph&) override;

  bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};

}  // namespace atlantis::invariantgraph
