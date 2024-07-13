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

  void init(InvariantGraph&, const InvariantNodeId&) override;

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      InvariantGraph&, propagation::SolverBase&) override;
};

}  // namespace atlantis::invariantgraph
