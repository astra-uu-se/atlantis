#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMaximumNode : public InvariantNode {
 public:
  ArrayIntMaximumNode(std::vector<VarNodeId>&& vars, VarNodeId output);

  ~ArrayIntMaximumNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void propagate(InvariantGraph& graph) override;

  [[nodiscard]] bool replace(InvariantGraph& graph) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
