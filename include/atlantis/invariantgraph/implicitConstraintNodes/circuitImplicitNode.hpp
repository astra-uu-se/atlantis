#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::invariantgraph {

class CircuitImplicitNode : public ImplicitConstraintNode {
 public:
  explicit CircuitImplicitNode(InvariantGraph&, std::vector<VarNodeId>&&);

  void init(const InvariantNodeId&) override;

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood()
      override;
};

}  // namespace atlantis::invariantgraph
