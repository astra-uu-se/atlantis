#include "atlantis/invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/randomNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::invariantgraph {

InvariantGraphRoot::InvariantGraphRoot(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& vars)
    : ImplicitConstraintNode(graph, std::move(vars)) {}

void InvariantGraphRoot::updateDomainTypes() {
  for (const auto& nId : outputVarNodes()) {
    nId->setDomainType(DomainType::DOM_NONE);
  }
}

void InvariantGraphRoot::registerNode(propagation::SolverBase&,
                                      SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(mappingId()));

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodes().size());

  for (const auto& vNode : outputVarNodes()) {
    assert(mapping.solverId(vNode->mappingId()) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(vNode->mappingId()), vNode->constDomain());
  }

  mapping.setNeighborhood(
      mappingId(), std::make_shared<search::neighborhoods::RandomNeighborhood>(
                std::move(searchVars)));
}

void InvariantGraphRoot::addSearchVarNode(VarNode& varNode) {
  markOutputTo(varNode.ptr(), true);
  assert(outputVarNodes().back().get() == &varNode);
}

std::ostream& InvariantGraphRoot::dotLangEdges(std::ostream& o) const {
  return o;
}

std::ostream& InvariantGraphRoot::dotLangEntry(std::ostream& o) const {
  return o;
}

std::string InvariantGraphRoot::dotLangIdentifier() const { return ""; }

}  // namespace atlantis::invariantgraph
