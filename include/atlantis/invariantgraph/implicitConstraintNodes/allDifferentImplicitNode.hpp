#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  explicit AllDifferentImplicitNode(InvariantGraph&, std::vector<VarNodeId>&&);

  void init(InvariantNodeId) override;

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood()
      override;
};

}  // namespace atlantis::invariantgraph
