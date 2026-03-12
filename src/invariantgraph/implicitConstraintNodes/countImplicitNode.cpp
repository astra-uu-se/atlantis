#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/countImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/allDifferentNonUniformNeighborhood.hpp"
#include "atlantis/search/neighborhoods/allDifferentUniformNeighborhood.hpp"
#include "atlantis/search/neighborhoods/countNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

CountImplicitNode::CountImplicitNode(InvariantGraph& graph,
                                     std::vector<VarNodeId>&& inputVars,
                                     const Int needle, const size_t amount)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _needle(needle),
      _amount(amount) {}

void CountImplicitNode::init(InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
  assert(std::ranges::all_of(
      outputVarNodeIds(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(outputVarNodeIds().front()).isIntVar() == invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void CountImplicitNode::updateDomainTypes() {
  for (const auto& nId : outputVarNodeIds()) {
    auto& varNode = invariantGraph().varNode(nId);
    varNode.setDomainType(DomainType::DOM_NONE);
  }
}

void CountImplicitNode::registerNode(propagation::SolverBase&,
                                     SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));
  assert(!outputVarNodeIds().empty());
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return invariantGraphConst().varNodeConst(vId).definingNodes().size() ==
               1 &&
           invariantGraphConst().varNodeConst(vId).outputOf() == id();
  }));

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& vId : outputVarNodeIds()) {
    const auto& varNode = invariantGraphConst().varNodeConst(vId);
    searchVars.emplace_back(mapping.solverId(vId), varNode.constDomain());
  }
  mapping.setNeighborhood(
      id(), std::make_shared<search::neighborhoods::CountNeighborhood>(
                std::move(searchVars), _needle, _amount));
}

std::string CountImplicitNode::dotLangIdentifier() const { return "count"; }

}  // namespace atlantis::invariantgraph
