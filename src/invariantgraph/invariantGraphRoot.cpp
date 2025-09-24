#include "atlantis/invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/randomNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::invariantgraph {

InvariantGraphRoot::InvariantGraphRoot(InvariantGraph& graph,
                                       std::vector<VarNodeId>&& vars)
    : ImplicitConstraintNode(graph, std::move(vars)) {}


void InvariantGraphRoot::updateDomainTypes() {
  for (const auto& nId : outputVarNodeIds()) {
    invariantGraph().varNode(nId).setDomainType(DomainType::DOM_NONE);
  }
}


void InvariantGraphRoot::registerNode(propagation::SolverBase&, SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    auto& node = invariantGraphConst().varNodeConst(nId);
    assert(mapping.solverId(nId) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(nId), node.constDomain());
  }

  mapping.setNeighborhood(id(), std::make_shared<search::neighborhoods::RandomNeighborhood>(
      std::move(searchVars)));
}

void InvariantGraphRoot::addSearchVarNode(VarNodeId vId) {
  markOutputTo(vId, true);
  assert(outputVarNodeIds().back() == vId);
}

std::ostream& InvariantGraphRoot::dotLangEdges(std::ostream& o) const {
  return o;
}

std::ostream& InvariantGraphRoot::dotLangEntry(std::ostream& o) const {
  return o;
}

std::string InvariantGraphRoot::dotLangIdentifier() const { return ""; }

}  // namespace atlantis::invariantgraph
