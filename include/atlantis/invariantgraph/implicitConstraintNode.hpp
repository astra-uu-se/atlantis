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

  /**
   * Take the neighborhood which is constructed in the registerNode
   * call out of this instance. Note, this transfers ownership (as indicated
   * by the usage of unique_ptr).
   *
   * Calling this method before calling registerNode will return a
   * nullptr. The same holds if this method is called multiple times. Only
   * the first call will return a neighborhood instance.
   *
   * The reason this does not return a reference, is because we want to be
   * able to delete the entire invariant graph after it has been applied to
   * the propagation solver. If a reference was returned here, that would
   * leave the reference dangling.
   *
   * @return The neighborhood corresponding to this implicit constraint.
   */
  [[nodiscard]] std::shared_ptr<search::neighborhoods::Neighborhood>
  neighborhood();

 protected:
  [[nodiscard]] virtual std::shared_ptr<search::neighborhoods::Neighborhood>
  createNeighborhood() = 0;
};
}  // namespace atlantis::invariantgraph
