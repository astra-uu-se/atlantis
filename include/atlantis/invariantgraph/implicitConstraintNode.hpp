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
 public:
  explicit ImplicitConstraintNode(InvariantGraph&, std::vector<VarNodeId>&&);

  void init(InvariantNodeId) override;

  virtual void updateDomainTypes() {};

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&,
                    SolverMapping&) const override = 0;
};
}  // namespace atlantis::invariantgraph
