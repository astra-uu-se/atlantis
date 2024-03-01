#include "atlantis/invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "atlantis/search/neighbourhoods/randomNeighbourhood.hpp"

namespace atlantis::invariantgraph {

InvariantGraphRoot::InvariantGraphRoot(std::vector<VarNodeId>&& vars)
    : ImplicitConstraintNode(std::move(vars)) {}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
InvariantGraphRoot::createNeighbourhood(propagation::SolverBase& solver,
                                        std::vector<search::SearchVar>&& vars) {
  return std::make_shared<search::neighbourhoods::RandomNeighbourhood>(
      std::move(vars), solver);
}

void InvariantGraphRoot::addSearchVarNode(VarNode& varNode) {
  markOutputTo(varNode);
  assert(outputVarNodeIds().back() == varNode.varNodeId());
}

}  // namespace atlantis::invariantgraph
