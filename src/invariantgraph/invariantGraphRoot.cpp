#include "invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "search/neighbourhoods/randomNeighbourhood.hpp"

search::neighbourhoods::Neighbourhood*
invariantgraph::InvariantGraphRoot::createNeighbourhood(
    Engine& engine, std::vector<search::SearchVariable> variables) {
  return new search::neighbourhoods::RandomNeighbourhood(std::move(variables),
                                                         engine);
}
