#include "atlantis/invariantgraph/implicitConstraintNodes/intLinEqImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/intLinEqNeighborhood.hpp"

namespace atlantis::invariantgraph {

IntLinEqImplicitNode::IntLinEqImplicitNode(
    InvariantGraph& graph, std::vector<Int>&& coeffs,
    std::vector<std::shared_ptr<VarNode>>&& inputVars, const Int offset)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _coeffs(std::move(coeffs)),
      _offset(offset) {
  assert(_coeffs.size() == outputVarNodes().size());
  assert(std::ranges::all_of(_coeffs,
                             [&](const Int c) { return std::abs(c) == 1; }));
}

void IntLinEqImplicitNode::init() {
  ImplicitConstraintNode::init();
  assert(std::ranges::all_of(
      outputVarNodes().begin(), outputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void IntLinEqImplicitNode::updateDomainTypes() {
  if (outputVarNodes().size() <= 1) {
    return;
  }

  for (const auto& nId : outputVarNodes()) {
    nId->setDomainType(DomainType::DOM_DOMAIN);
  }
}

void IntLinEqImplicitNode::registerNode(propagation::SolverBase&,
                                        SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(ptrConst()));
  if (outputVarNodes().size() <= 1) {
    return;
  }

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodes().size());

  for (const auto& varNode : outputVarNodes()) {
    assert(mapping.solverId(varNode) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(varNode), varNode->constDomain());
  }

  mapping.setNeighborhood(
      ptrConst(), std::make_shared<search::neighborhoods::IntLinEqNeighborhood>(
                std::vector<Int>{_coeffs}, std::move(searchVars), _offset));
}

std::string IntLinEqImplicitNode::dotLangIdentifier() const {
  return "int_lin_eq";
}

}  // namespace atlantis::invariantgraph
