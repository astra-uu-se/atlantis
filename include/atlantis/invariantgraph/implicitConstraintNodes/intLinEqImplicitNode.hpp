#pragma once

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::invariantgraph {

class IntLinEqImplicitNode : public ImplicitConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _bound;

 public:
  explicit IntLinEqImplicitNode(std::vector<Int>&& coeffs,
                                std::vector<VarNodeId>&&, Int bound);

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      InvariantGraph&, propagation::SolverBase&) override;
};

}  // namespace atlantis::invariantgraph
