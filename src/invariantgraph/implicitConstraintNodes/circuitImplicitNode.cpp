#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/iInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/circuitNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

CircuitImplicitNode::CircuitImplicitNode(IInvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         Int offset)
    : ImplicitConstraintNode(graph, std::move(vars)), _offset(offset) {
  assert(InvariantNode::outputVarNodeIds().size() > 1);
}

void CircuitImplicitNode::init(InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

std::shared_ptr<search::neighborhoods::Neighborhood>
CircuitImplicitNode::createNeighborhood() {
  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());
  std::vector<Int> freeIndices;
  freeIndices.reserve(outputVarNodeIds().size());
  for (const auto& nId : outputVarNodeIds()) {
    const auto& varNode = invariantGraphConst().varNodeConst(nId);
    if (varNode.isFixed()) {
      freeIndices.emplace_back(varNode.constDomain()->lowerBound());
    }
  }

  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    auto& varNode = invariantGraph().varNode(outputVarNodeIds().at(i));
    assert(varNode.varId() != propagation::NULL_ID);
    searchVars.emplace_back(varNode.varId(), varNode.domain());

    if (varNode.constDomain()->isFixed()) {
      varNode.setDomainType(DomainType::DOM_NONE);
      continue;
    }

    bool enforceDomain = false;
    for (Int val = 0; val <= static_cast<Int>(outputVarNodeIds().size());
         ++val) {
      if (val == (static_cast<Int>(i) - 1) ||
          std::ranges::any_of(freeIndices.begin(), freeIndices.end(),
                              [&](const Int& index) { return index == val; })) {
        continue;
      }
      if (!varNode.constDomain()->contains(val)) {
        enforceDomain = true;
        break;
      }
    }
    varNode.setDomainType(enforceDomain ? DomainType::DOM_DOMAIN
                                        : DomainType::DOM_NONE);
  }
  return std::make_shared<search::neighborhoods::CircuitNeighborhood>(
      std::move(searchVars), _offset);
}

std::string CircuitImplicitNode::dotLangIdentifier() const { return "circuit"; }

}  // namespace atlantis::invariantgraph
