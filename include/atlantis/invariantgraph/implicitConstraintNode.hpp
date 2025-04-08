#pragma once

#include <memory>

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::search::neighborhoods {
class Neighborhood;
}

namespace atlantis::invariantgraph {

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation solver.
 */
class ImplicitConstraintNode : public InvariantNode {
  std::shared_ptr<search::neighborhoods::Neighborhood> _neighborhood{nullptr};

 public:
  explicit ImplicitConstraintNode(InvariantGraph&, std::vector<VarNodeId>&&);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] std::shared_ptr<search::neighborhoods::Neighborhood>
  neighborhood();

 protected:
  [[nodiscard]] virtual std::shared_ptr<search::neighborhoods::Neighborhood>
  createNeighborhood() = 0;
};
}  // namespace atlantis::invariantgraph
