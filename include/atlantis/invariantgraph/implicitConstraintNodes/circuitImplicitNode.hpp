#pragma once

#include <numeric>

#include "invariantgraph/implicitConstraintNode.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::invariantgraph {

class CircuitImplicitNode : public ImplicitConstraintNode {
 public:
  explicit CircuitImplicitNode(std::vector<VarNodeId>&&);

  ~CircuitImplicitNode() override = default;

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      propagation::SolverBase& solver,
      std::vector<search::SearchVar>&&) override;
};

}  // namespace atlantis::invariantgraph