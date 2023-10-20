#include "invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "search/neighbourhoods/randomNeighbourhood.hpp"

std::shared_ptr<search::neighbourhoods::Neighbourhood>
invariantgraph::InvariantGraphRoot::createNeighbourhood(
    Engine& engine, std::vector<search::SearchVariable>&& variables) {
  return std::make_shared<search::neighbourhoods::RandomNeighbourhood>(
      std::move(variables), engine);
}

void invariantgraph::InvariantGraphRoot::addSearchVarNode(VarNode& varNode) {
  markOutputTo(varNode);
  assert(outputVarNodeIds().back() == varNode.varNodeId());
}
