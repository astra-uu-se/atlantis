#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::invariantgraph {

class CircuitImplicitNode : public ImplicitConstraintNode {
 public:
  explicit CircuitImplicitNode(std::vector<VarNodeId>&&);

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      InvariantGraph& invariantGraph, propagation::SolverBase& solver) override;
};

}  // namespace atlantis::invariantgraph
