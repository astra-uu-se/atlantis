#include "atlantis/invariantgraph/invariantGraphRoot.hpp"

#include <utility>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/search/neighbourhoods/randomNeighbourhood.hpp"

namespace atlantis::invariantgraph {

InvariantGraphRoot::InvariantGraphRoot(std::vector<VarNodeId>&& vars)
    : ImplicitConstraintNode(std::move(vars)) {}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
InvariantGraphRoot::createNeighbourhood(InvariantGraph& graph,
                                        propagation::SolverBase& solver) {
  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    SearchDomain dom = graph.varNode(outputVarNodeIds().front()).constDomain();
    auto& node = graph.varNode(nId);
    assert(node.varId() != propagation::NULL_ID);
    searchVars.emplace_back(node.varId(), std::move(dom));
    node.setDomainType(VarNode::DomainType::NONE);
  }

  return std::make_shared<search::neighbourhoods::RandomNeighbourhood>(
      std::move(searchVars), solver);
}

void InvariantGraphRoot::addSearchVarNode(VarNode& varNode) {
  markOutputTo(varNode);
  assert(outputVarNodeIds().back() == varNode.varNodeId());
}

std::ostream& InvariantGraphRoot::dotLangEdges(std::ostream& o) const {
  return o;
}

std::ostream& InvariantGraphRoot::dotLangEntry(std::ostream& o) const {
  return o;
}

std::string InvariantGraphRoot::dotLangIdentifier() const { return ""; }

}  // namespace atlantis::invariantgraph
