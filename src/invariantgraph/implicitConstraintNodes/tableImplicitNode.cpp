#include "atlantis/invariantgraph/implicitConstraintNodes/tableImplicitNode.hpp"

#include <algorithm>
#include <limits>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/search/neighborhoods/tableNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

TableImplicitNode::TableImplicitNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& inputVars, std::vector<std::vector<Int>>&& table)
    : ImplicitConstraintNode(graph, std::move(inputVars)), _table(std::move(table)) {}

void TableImplicitNode::init(const InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
}

void TableImplicitNode::updateDomainTypes() {
  if (outputVarNodeIds().size() <= 1) {
    return;
  }
  assert(!outputVarNodeIds().empty());
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return invariantGraphConst().varNodeConst(vId).definingNodes().size() ==
               1 &&
           invariantGraphConst().varNodeConst(vId).outputOf() == id();
  }));

  for (const auto& nId : outputVarNodeIds()) {
    invariantGraph().varNode(nId).setDomainType(DomainType::DOM_NONE);
  }
}

void TableImplicitNode::registerNode(propagation::SolverBase&,
                                            SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));

  if (outputVarNodeIds().size() <= 1) {
    return;
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return invariantGraphConst().varNodeConst(vId).definingNodes().size() ==
               1 &&
           invariantGraphConst().varNodeConst(vId).outputOf() == id();
  }));

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    const auto& varNode = invariantGraphConst().varNodeConst(nId);
    assert(mapping.solverId(nId) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(nId), varNode.constDomain());
  }
  mapping.setNeighborhood(
      id(), std::make_shared<
                search::neighborhoods::TableNeighborhood>(
                std::move(searchVars), std::vector<std::vector<Int>>{_table}));
}

std::string TableImplicitNode::dotLangIdentifier() const {
  return "table";
}

}  // namespace atlantis::invariantgraph
