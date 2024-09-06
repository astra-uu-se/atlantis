#pragma once

#include <memory>

#include "atlantis/invariantgraph/iInvariantNode.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"

namespace atlantis::invariantgraph {

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation solver.
 */
class IImplicitConstraintNode : public virtual IInvariantNode {
 protected:
  virtual std::shared_ptr<search::neighbourhoods::Neighbourhood>
  createNeighbourhood() = 0;

 public:
  /**
   * Calling this method before calling registerNode will return a
   * nullptr. The same holds if this method is called multiple times. Only
   * the first call will return a neighbourhood instance.
   *
   * @return The neighbourhood corresponding to this implicit constraint.
   */
  [[nodiscard]] virtual std::shared_ptr<search::neighbourhoods::Neighbourhood>
  neighbourhood() = 0;
};
}  // namespace atlantis::invariantgraph
