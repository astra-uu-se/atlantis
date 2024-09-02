#include "atlantis/invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "atlantis/invariantgraph/iInvariantGraph.hpp"
#include "atlantis/search/neighbourhoods/randomNeighbourhood.hpp"

namespace atlantis::invariantgraph {

InvariantGraphRoot::InvariantGraphRoot(IInvariantGraph& graph,
                                       std::vector<VarNodeId>&& vars)
    : ImplicitConstraintNode(graph, std::move(vars)) {}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
InvariantGraphRoot::createNeighbourhood() {
  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    SearchDomain dom = invariantGraphConst()
                           .varNodeConst(outputVarNodeIds().front())
                           .constDomain();
    auto& node = invariantGraph().varNode(nId);
    assert(node.varId() != propagation::NULL_ID);
    searchVars.emplace_back(node.varId(), std::move(dom));
    node.setDomainType(VarNode::DomainType::NONE);
  }

  return std::make_shared<search::neighbourhoods::RandomNeighbourhood>(
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
