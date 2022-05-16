#include "invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "search/neighbourhoods/randomNeighbourhood.hpp"

search::neighbourhoods::Neighbourhood*
invariantgraph::InvariantGraphRoot::createNeighbourhood(
    Engine& engine, std::vector<search::SearchVariable> variables) {
  return new search::neighbourhoods::RandomNeighbourhood(std::move(variables),
                                                         engine);
}

void invariantgraph::InvariantGraphRoot::addSearchVariable(VariableNode* node) {
  assert(node->definedBy() == nullptr);
  addDefinedVariable(node);
  assert(definedVariables().back() == node);
  assert(node->definedBy() == this);
}
