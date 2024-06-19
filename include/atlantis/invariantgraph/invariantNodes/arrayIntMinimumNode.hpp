#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMinimumNode : public InvariantNode {
 public:
  explicit ArrayIntMinimumNode(VarNodeId a, VarNodeId b, VarNodeId output);

  explicit ArrayIntMinimumNode(std::vector<VarNodeId>&& vars, VarNodeId output);

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void updateState(InvariantGraph&) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;
};
}  // namespace atlantis::invariantgraph
