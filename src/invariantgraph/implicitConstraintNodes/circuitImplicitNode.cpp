#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"

#include <numeric>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/iInvariantGraph.hpp"
#include "atlantis/search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::invariantgraph {

CircuitImplicitNode::CircuitImplicitNode(IInvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         Int offset)
    : ImplicitConstraintNode(graph, std::move(vars)), _offset(offset) {
  assert(outputVarNodeIds().size() > 1);
}

void CircuitImplicitNode::init(InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
  assert(
      std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                  [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
CircuitImplicitNode::createNeighbourhood() {
  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());
  std::vector<Int> freeIndices;
  freeIndices.reserve(outputVarNodeIds().size());
  for (const auto& nId : outputVarNodeIds()) {
    const auto& varNode = invariantGraphConst().varNodeConst(nId);
    if (varNode.isFixed()) {
      freeIndices.emplace_back(varNode.constDomain().lowerBound());
    }
  }

  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    auto& varNode = invariantGraph().varNode(outputVarNodeIds().at(i));
    assert(varNode.varId() != propagation::NULL_ID);
    if (varNode.constDomain().isFixed()) {
      const Int val = varNode.constDomain().lowerBound();
      searchVars.emplace_back(varNode.varId(), SearchDomain{val, val});
      varNode.setDomainType(VarNode::DomainType::NONE);
      continue;
    }

    searchVars.emplace_back(
        varNode.varId(),
        SearchDomain{1, static_cast<Int>(outputVarNodeIds().size())});

    bool enforceDomain = false;
    for (Int val = 0; val <= static_cast<Int>(outputVarNodeIds().size());
         ++val) {
      if (val == (static_cast<Int>(i) - 1) ||
          std::any_of(freeIndices.begin(), freeIndices.end(),
                      [&](const Int& i) { return i == val; })) {
        continue;
      }
      if (!varNode.constDomain().contains(val)) {
        enforceDomain = true;
        break;
      }
    }
    varNode.setDomainType(enforceDomain ? VarNode::DomainType::DOMAIN
                                        : VarNode::DomainType::NONE);
  }
  return std::make_shared<search::neighbourhoods::CircuitNeighbourhood>(
      std::move(searchVars), _offset);
}

std::string CircuitImplicitNode::dotLangIdentifier() const { return "circuit"; }

}  // namespace atlantis::invariantgraph
