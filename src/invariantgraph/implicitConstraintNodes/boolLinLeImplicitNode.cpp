#include "atlantis/invariantgraph/implicitConstraintNodes/boolLinLeImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/binaryLinLeNeighborhood.hpp"

namespace atlantis::invariantgraph {

BoolLinLeImplicitNode::BoolLinLeImplicitNode(
    InvariantGraph& graph, std::vector<Int>&& coeffs,
    std::vector<std::shared_ptr<VarNode>>&& inputVars, const Int bound)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _coeffs(std::move(coeffs)),
      _bound(bound) {
  assert(_coeffs.size() == outputVarNodes().size());
}

void BoolLinLeImplicitNode::init() {
  ImplicitConstraintNode::init();
  assert(std::ranges::none_of(
      outputVarNodes(),
      [&](const std::shared_ptr<VarNode>& var) { return var->isIntVar(); }));
}

void BoolLinLeImplicitNode::updateDomainTypes() {
  Int sum = 0;
  for (size_t i = 0; i < outputVarNodes().size(); ++i) {
    const Int v1 = _coeffs[i] * outputVarNodes()[i]->lowerBound();
    const Int v2 = _coeffs[i] * outputVarNodes()[i]->upperBound();
    // TODO: check for overflow:
    sum += std::max(v1, v2);
  }
  if (sum <= _bound) {
    return;
  }
  for (const auto& outVar : outputVarNodes()) {
    outVar->setDomainType(DomainType::DOM_DOMAIN);
  }
}

void BoolLinLeImplicitNode::registerNode(propagation::SolverBase&,
                                         SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(ptrConst()));
  Int sum = 0;
  for (size_t i = 0; i < outputVarNodes().size(); ++i) {
    const Int v1 =
        _coeffs[i] *
        outputVarNodes()[i]->lowerBound();
    const Int v2 =
        _coeffs[i] *
        outputVarNodes()[i]->upperBound();
    sum += std::max(v1, v2);
  }
  if (sum <= _bound) {
    return;
  }

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodes().size());

  for (const auto& vNode : outputVarNodes()) {
    auto& varNode = vNode;
    assert(mapping.solverId(vNode) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(vNode), varNode->constDomain());
  }

  mapping.setNeighborhood(
      ptrConst(),
      std::make_shared<search::neighborhoods::BinaryLinLeNeighborhood<true>>(
          std::vector<Int>{_coeffs}, std::move(searchVars), _bound));
}

std::string BoolLinLeImplicitNode::dotLangIdentifier() const {
  return "bool_lin_le";
}

}  // namespace atlantis::invariantgraph
