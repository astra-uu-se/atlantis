#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class CircuitNode : public ViolationInvariantNode {
 public:
  explicit CircuitNode(std::vector<VarNodeId>&&);

  [[nodiscard]] bool canBeMadeImplicit(const InvariantGraph&) const override;

  [[nodiscard]] bool makeImplicit(InvariantGraph& graph) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
