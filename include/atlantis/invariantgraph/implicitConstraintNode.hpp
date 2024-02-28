#pragma once

#include <cassert>
#include <vector>

#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"

namespace atlantis::invariantgraph {

class InvariantGraph;  // Forward declaration

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation solver.
 */
class ImplicitConstraintNode : public InvariantNode {
 private:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> _neighbourhood{
      nullptr};

 public:
  explicit ImplicitConstraintNode(std::vector<VarNodeId>&& outputVarNodeIds);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  /**
   * Take the neighbourhood which is constructed in the registerNode
   * call out of this instance. Note, this transfers ownership (as indicated
   * by the usage of unique_ptr).
   *
   * Calling this method before calling registerNode will return a
   * nullptr. The same holds if this method is called multiple times. Only
   * the first call will return a neighbourhood instance.
   *
   * The reason this does not return a reference, is because we want to be
   * able to delete the entire invariant graph after it has been applied to
   * the propagation solver. If a reference was returned here, that would
   * leave the reference dangling.
   *
   * @return The neighbourhood corresponding to this implicit constraint.
   */
  [[nodiscard]] std::shared_ptr<search::neighbourhoods::Neighbourhood>
  neighbourhood() noexcept;

 protected:
  virtual std::shared_ptr<search::neighbourhoods::Neighbourhood>
  createNeighbourhood(propagation::SolverBase& solver,
                      std::vector<search::SearchVar>&& vars) = 0;
};
}  // namespace atlantis::invariantgraph
