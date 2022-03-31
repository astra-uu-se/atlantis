#include "invariantgraph/invariantGraphRoot.hpp"

#include "search/neighbourhoods/randomNeighbourhood.hpp"

search::neighbourhoods::Neighbourhood*
invariantgraph::InvariantGraphRoot::createNeighbourhood(
    Engine& engine, const std::vector<VarId>& varIds) {
  return new search::neighbourhoods::RandomNeighbourhood(varIds, engine);
}
