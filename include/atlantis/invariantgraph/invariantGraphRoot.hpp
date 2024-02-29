#pragma once

#include <utility>

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/search/neighbourhoods/randomNeighbourhood.hpp"

namespace atlantis::invariantgraph {

/**
 * Given all variable nodes need to be defined by a a InvariantNode, this
 * class "defines" the search variables so their nodes can be created and
 * registered with the solver.
 */
class InvariantGraphRoot : public ImplicitConstraintNode {
 public:
  explicit InvariantGraphRoot(std::vector<VarNodeId>&& vars = {})
      : ImplicitConstraintNode(std::move(vars)) {}

  ~InvariantGraphRoot() override = default;

  void addSearchVarNode(VarNode&);

 protected:
  std::shared_ptr<search::neighbourhoods::Neighbourhood> createNeighbourhood(
      propagation::SolverBase& solver,
      std::vector<search::SearchVar>&& vars) override;
};

}  // namespace atlantis::invariantgraph
