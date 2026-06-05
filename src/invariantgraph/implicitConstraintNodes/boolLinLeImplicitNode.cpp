#include "atlantis/invariantgraph/implicitConstraintNodes/boolLinLeImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/binaryLinLeNeighborhood.hpp"

namespace atlantis::invariantgraph {

BoolLinLeImplicitNode::BoolLinLeImplicitNode(InvariantGraph& graph,
                                             std::vector<Int>&& coeffs,
                                             std::vector<VarNodeId>&& inputVars,
                                             const Int bound)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _coeffs(std::move(coeffs)),
      _bound(bound) {
  assert(_coeffs.size() == outputVarNodeIds().size());
}

void BoolLinLeImplicitNode::init(const InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
  assert(std::ranges::none_of(
      outputVarNodeIds(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BoolLinLeImplicitNode::updateDomainTypes() {
  Int sum = 0;
  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    const Int v1 =
        _coeffs[i] *
        invariantGraphConst().varNodeConst(outputVarNodeIds()[i]).lowerBound();
    const Int v2 =
        _coeffs[i] *
        invariantGraphConst().varNodeConst(outputVarNodeIds()[i]).upperBound();
    // TODO: check for overflow:
    sum += std::max(v1, v2);
  }
  if (sum <= _bound) {
    return;
  }
  for (const auto& nId : outputVarNodeIds()) {
    invariantGraph().varNode(nId).setDomainType(DomainType::DOM_DOMAIN);
  }
}

void BoolLinLeImplicitNode::registerNode(propagation::SolverBase&,
                                         SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));
  Int sum = 0;
  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    const Int v1 =
        _coeffs[i] *
        invariantGraphConst().varNodeConst(outputVarNodeIds()[i]).lowerBound();
    const Int v2 =
        _coeffs[i] *
        invariantGraphConst().varNodeConst(outputVarNodeIds()[i]).upperBound();
    sum += std::max(v1, v2);
  }
  if (sum <= _bound) {
    return;
  }

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    auto& varNode = invariantGraphConst().varNodeConst(nId);
    assert(mapping.solverId(nId) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(nId), varNode.constDomain());
  }

  mapping.setNeighborhood(
      id(),
      std::make_shared<search::neighborhoods::BinaryLinLeNeighborhood<true>>(
          std::vector<Int>{_coeffs}, std::move(searchVars), _bound));
}

std::string BoolLinLeImplicitNode::dotLangIdentifier() const {
  return "bool_lin_le";
}

}  // namespace atlantis::invariantgraph
