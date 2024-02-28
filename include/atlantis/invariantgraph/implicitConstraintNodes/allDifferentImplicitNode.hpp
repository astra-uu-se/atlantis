#pragma once

#include <numeric>

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"
#include "atlantis/search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::invariantgraph {

class AllDifferentImplicitNode : public ImplicitConstraintNode {
 public:
  explicit AllDifferentImplicitNode(std::vector<VarNodeId>&&);

  ~AllDifferentImplicitNode() override = default;

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      propagation::SolverBase&, std::vector<search::SearchVar>&&) override;
};

}  // namespace atlantis::invariantgraph
