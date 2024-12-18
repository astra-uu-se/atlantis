#include "atlantis/invariantgraph/implicitConstraintNodes/intLinEqImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/iInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/intLinEqNeighborhood.hpp"

namespace atlantis::invariantgraph {

IntLinEqImplicitNode::IntLinEqImplicitNode(IInvariantGraph& graph,
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

std::shared_ptr<search::neighborhoods::Neighborhood>
IntLinEqImplicitNode::createNeighborhood() {
  if (outputVarNodeIds().size() <= 1) {
    return nullptr;
  }

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    auto& varNode = invariantGraph().varNode(nId);
    assert(varNode.varId() != propagation::NULL_ID);
    searchVars.emplace_back(varNode.varId(), varNode.domain());
    varNode.setDomainType(DomainType::DOM_DOMAIN);
  }

  return std::make_shared<search::neighborhoods::IntLinEqNeighborhood>(
      std::vector<Int>{_coeffs}, std::move(searchVars), _offset);
}

std::string IntLinEqImplicitNode::dotLangIdentifier() const {
  return "int_lin_eq";
}

}  // namespace atlantis::invariantgraph
