#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"

#include <numeric>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::invariantgraph {

CircuitImplicitNode::CircuitImplicitNode(std::vector<VarNodeId>&& vars)
    : ImplicitConstraintNode(std::move(vars)) {
  assert(outputVarNodeIds().size() > 1);
}

void CircuitImplicitNode::init(InvariantGraph& graph,
                               const InvariantNodeId& id) {
  ImplicitConstraintNode::init(graph, id);
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
CircuitImplicitNode::createNeighbourhood(InvariantGraph& graph,
                                         propagation::SolverBase&) {
  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());
  SearchDomain freeIndices(1, static_cast<Int>(outputVarNodeIds().size()));
  for (const auto& nId : outputVarNodeIds()) {
    const auto& varNode = graph.varNodeConst(nId);
    if (varNode.isFixed()) {
      freeIndices.remove(varNode.constDomain().lowerBound());
    }
  }

  for (const auto& nId : outputVarNodeIds()) {
    auto& varNode = graph.varNode(nId);
    assert(varNode.varId() != propagation::NULL_ID);
    if (varNode.constDomain().isFixed()) {
      const Int val = varNode.constDomain().lowerBound();
      searchVars.emplace_back(varNode.varId(), SearchDomain{val, val});
      continue;
    }

    searchVars.emplace_back(
        varNode.varId(),
        SearchDomain{1, static_cast<Int>(outputVarNodeIds().size())});

    if (varNode.constDomain() == freeIndices) {
      varNode.setDomainType(VarNode::DomainType::NONE);
    } else {
      varNode.setDomainType(VarNode::DomainType::DOMAIN);
    }
  }
  return std::make_shared<search::neighbourhoods::CircuitNeighbourhood>(
      std::move(searchVars));
}

}  // namespace atlantis::invariantgraph
