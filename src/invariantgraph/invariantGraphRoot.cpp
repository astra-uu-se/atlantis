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

std::shared_ptr<search::neighborhoods::Neighborhood>
InvariantGraphRoot::createNeighborhood() {
  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    auto dom = invariantGraphConst()
                   .varNodeConst(outputVarNodeIds().front())
                   .constDomain();
    auto& node = invariantGraph().varNode(nId);
    assert(node.varId() != propagation::NULL_ID);
    searchVars.emplace_back(node.varId(), dom);
    node.setDomainType(DomainType::DOM_NONE);
  }

  return std::make_shared<search::neighborhoods::RandomNeighborhood>(
      std::move(searchVars));
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
