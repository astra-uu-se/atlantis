#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolOrNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolOrNode(VarNodeId a, VarNodeId b, VarNodeId output);

  ArrayBoolOrNode(VarNodeId a, VarNodeId b, bool shouldHold = true);

  ArrayBoolOrNode(std::vector<VarNodeId>&& inputs, VarNodeId output);

  ArrayBoolOrNode(std::vector<VarNodeId>&& inputs, bool shouldHold = true);

  void updateState(InvariantGraph&) override;

  bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};

}  // namespace atlantis::invariantgraph
