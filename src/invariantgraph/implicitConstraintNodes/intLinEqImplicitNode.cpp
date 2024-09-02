#include "atlantis/invariantgraph/implicitConstraintNodes/intLinEqImplicitNode.hpp"

#include <numeric>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/search/neighbourhoods/intLinEqNeighbourhood.hpp"

namespace atlantis::invariantgraph {

IntLinEqImplicitNode::IntLinEqImplicitNode(std::vector<Int>&& coeffs,
                                           std::vector<VarNodeId>&& inputVars,
                                           Int offset)
    : ImplicitConstraintNode(std::move(inputVars)),
      _coeffs(std::move(coeffs)),
      _offset(offset) {}

void IntLinEqImplicitNode::init(InvariantGraph& graph,
                                const InvariantNodeId& id) {
  ImplicitConstraintNode::init(graph, id);
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
IntLinEqImplicitNode::createNeighbourhood(InvariantGraph& graph,
                                          propagation::SolverBase& solver) {
  if (outputVarNodeIds().size() <= 1) {
    return nullptr;
  }

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    auto& varNode = graph.varNode(nId);
    assert(varNode.varId() != propagation::NULL_ID);
    searchVars.emplace_back(varNode.varId(),
                            SearchDomain{varNode.constDomain().lowerBound(),
                                         varNode.constDomain().upperBound()});
    varNode.setDomainType(VarNode::DomainType::DOMAIN);
  }

  return std::make_shared<search::neighbourhoods::IntLinEqNeighbourhood>(
      std::vector<Int>{_coeffs}, std::move(searchVars), _offset, solver);
}

std::string IntLinEqImplicitNode::dotLangIdentifier() const {
  return "int_lin_eq";
}

}  // namespace atlantis::invariantgraph
