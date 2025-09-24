#include "atlantis/invariantgraph/implicitConstraintNodes/intLinEqImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/intLinEqNeighborhood.hpp"

namespace atlantis::invariantgraph {

IntLinEqImplicitNode::IntLinEqImplicitNode(InvariantGraph& graph,
                                           std::vector<Int>&& coeffs,
                                           std::vector<VarNodeId>&& inputVars,
                                           Int offset)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _coeffs(std::move(coeffs)),
      _offset(offset) {}

void IntLinEqImplicitNode::init(InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void IntLinEqImplicitNode::updateDomainTypes() {
  if (outputVarNodeIds().size() <= 1) {
    return;
  }

  for (const auto& nId : outputVarNodeIds()) {
    invariantGraph().varNode(nId).setDomainType(DomainType::DOM_DOMAIN);
  }
}


void
IntLinEqImplicitNode::registerNode(propagation::SolverBase&, SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));
  if (outputVarNodeIds().size() <= 1) {
    return;
  }

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    auto& varNode = invariantGraphConst().varNodeConst(nId);
    assert(mapping.solverId(nId) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(nId), varNode.constDomain());
  }

  mapping.setNeighborhood(id(), std::make_shared<search::neighborhoods::IntLinEqNeighborhood>(
      std::vector<Int>{_coeffs}, std::move(searchVars), _offset));
}

std::string IntLinEqImplicitNode::dotLangIdentifier() const {
  return "int_lin_eq";
}

}  // namespace atlantis::invariantgraph
