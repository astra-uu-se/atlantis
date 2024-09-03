#pragma once

#include <memory>

#include "atlantis/invariantgraph/iInvariantNode.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"

namespace atlantis::invariantgraph {

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation solver.
 */
class IImplicitConstraintNode : public virtual IInvariantNode {
  IImplicitConstraintNode() = delete;
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
  [[nodiscard]] virtual std::shared_ptr<search::neighbourhoods::Neighbourhood>
  neighbourhood() = 0;

 protected:
  virtual std::shared_ptr<search::neighbourhoods::Neighbourhood>
  createNeighbourhood() = 0;
};
}  // namespace atlantis::invariantgraph
