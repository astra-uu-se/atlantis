#include "atlantis/invariantgraph/implicitConstraintNodes/linLeImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/binaryLinLeNeighborhood.hpp"
#include "atlantis/search/neighborhoods/intLinLeNeighborhood.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

LinLeImplicitNode::LinLeImplicitNode(InvariantGraph& graph,
                                           std::vector<Int>&& coeffs,
                                           std::vector<VarNodeId>&& inputVars,
                                           Int bound)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _coeffs(std::move(coeffs)),
      _bound(bound),
_isBinary(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
  const auto& vNode = invariantGraphConst().varNodeConst(vId);
  return !vNode.isIntVar() || (0 <= vNode.lowerBound() && vNode.upperBound() <= 1);
}))
{
  assert(_coeffs.size() == outputVarNodeIds().size());
}

void LinLeImplicitNode::init(const InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
}

void LinLeImplicitNode::updateDomainTypes() {
  for (const auto& vId : outputVarNodeIds()) {
    auto& vNode = invariantGraph().varNode(vId);
    vNode.setDomainType(vNode.constDomain()->isInterval() ? DomainType::DOM_NONE : DomainType::DOM_DOMAIN);
  }
}

void LinLeImplicitNode::registerNode(propagation::SolverBase&,
                                        SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(id()));
  Int total = 0;
  for (size_t i = 0; i < outputVarNodeIds().size(); ++i) {
    const Int lb = invariantGraphConst().varNodeConst(outputVarNodeIds()[i]).lowerBound();
    const Int ub = invariantGraphConst().varNodeConst(outputVarNodeIds()[i]).upperBound();
    const Int val1 = overflow::saturatingMul(_coeffs[i], lb);
    const Int val2 = overflow::saturatingMul(_coeffs[i], ub);
    total = overflow::saturatingAdd(total, std::max(val1, val2));
  }
  if (total <= _bound) {
    return;
  }

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodeIds().size());

  for (const auto& nId : outputVarNodeIds()) {
    auto& varNode = invariantGraphConst().varNodeConst(nId);
    assert(mapping.solverId(nId) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(nId), varNode.constDomain());
  }

  if (_isBinary) {
    if (invariantGraphConst().varNodeConst(outputVarNodeIds().front()).isIntVar()) {
      mapping.setNeighborhood(
      id(), std::make_shared<search::neighborhoods::BinaryLinLeNeighborhood<false>>(
                std::vector<Int>{_coeffs}, std::move(searchVars), _bound));
      return;
    }
    mapping.setNeighborhood(
    id(), std::make_shared<search::neighborhoods::BinaryLinLeNeighborhood<true>>(
              std::vector<Int>{_coeffs}, std::move(searchVars), _bound));
    return;
  }
  mapping.setNeighborhood(
      id(), std::make_shared<search::neighborhoods::IntLinLeNeighborhood>(
                std::vector<Int>{_coeffs}, std::move(searchVars), _bound));
}

std::string LinLeImplicitNode::dotLangIdentifier() const {
  return "lin_le";
}

}  // namespace atlantis::invariantgraph
