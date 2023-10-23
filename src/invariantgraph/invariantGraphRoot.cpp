#include "invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "search/neighbourhoods/randomNeighbourhood.hpp"

namespace atlantis::invariantgraph {

std::shared_ptr<search::neighbourhoods::Neighbourhood>
InvariantGraphRoot::createNeighbourhood(
    propagation::Engine& engine,
    std::vector<search::SearchVariable>&& variables) {
  return std::make_shared<search::neighbourhoods::RandomNeighbourhood>(
      std::move(variables), engine);
}

void InvariantGraphRoot::addSearchVarNode(VarNode& varNode) {
  markOutputTo(varNode);
  assert(outputVarNodeIds().back() == varNode.varNodeId());
}

}  // namespace atlantis::invariantgraph