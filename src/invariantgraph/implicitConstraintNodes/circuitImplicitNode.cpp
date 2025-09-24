#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/circuitNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

CircuitImplicitNode::CircuitImplicitNode(InvariantGraph& graph,
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

void CircuitImplicitNode::updateDomainTypes() {
    std::vector<Int> freeIndices;
    freeIndices.reserve(outputVarNodeIds().size());
    for (const auto& nId : outputVarNodeIds()) {
      const auto& varNode = invariantGraphConst().varNodeConst(nId);
      if (varNode.isFixed()) {
        freeIndices.emplace_back(varNode.constDomain()->lowerBound());
      }
    }

    for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
      const auto vId = outputVarNodeIds().at(i);
      auto& varNode = invariantGraph().varNode(vId);
      assert(vId != propagation::NULL_ID);

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
  }

void
CircuitImplicitNode::registerNode(propagation::SolverBase&, SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));

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

  for (unsigned long vId : outputVarNodeIds()) {
    const auto& varNode = invariantGraphConst().varNodeConst(vId);
    assert(vId != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(vId), varNode.constDomain());
  }
  mapping.setNeighborhood(id(), std::make_shared<search::neighborhoods::CircuitNeighborhood>(
      std::move(searchVars), _offset));
}

std::string CircuitImplicitNode::dotLangIdentifier() const { return "circuit"; }

}  // namespace atlantis::invariantgraph
