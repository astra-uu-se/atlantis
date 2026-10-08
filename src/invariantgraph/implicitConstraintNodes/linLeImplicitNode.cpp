#include "atlantis/invariantgraph/implicitConstraintNodes/linLeImplicitNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/binaryLinLeNeighborhood.hpp"
#include "atlantis/search/neighborhoods/intLinLeNeighborhood.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

LinLeImplicitNode::LinLeImplicitNode(
    InvariantGraph& graph, std::vector<Int>&& coeffs,
    std::vector<std::shared_ptr<VarNode>>&& inputVars, const Int bound)
    : ImplicitConstraintNode(graph, std::move(inputVars)),
      _coeffs(std::move(coeffs)),
      _bound(bound),
      _isBinary(std::ranges::all_of(
          outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
            const auto& vNode = vId;
            return !vNode->isIntVar() ||
                   (0 <= vNode->lowerBound() && vNode->upperBound() <= 1);
          })) {
  assert(_coeffs.size() == outputVarNodes().size());
}

void LinLeImplicitNode::init() { ImplicitConstraintNode::init(); }

void LinLeImplicitNode::updateDomainTypes() {
  for (const auto& vId : outputVarNodes()) {
    auto& vNode = vId;
    vNode->setDomainType(vNode->constDomain()->isInterval()
                            ? DomainType::DOM_NONE
                            : DomainType::DOM_DOMAIN);
  }
}

void LinLeImplicitNode::registerNode(propagation::SolverBase&,
                                     SolverMapping& mapping) const {
  assert(!mapping.hasNeighborhood(ptrConst()));
  Int total = 0;
  for (size_t i = 0; i < outputVarNodes().size(); ++i) {
    const Int lb =
        outputVarNodes()[i]->lowerBound();
    const Int ub =
        outputVarNodes()[i]->upperBound();
    const Int val1 = overflow::saturatingMul(_coeffs[i], lb);
    const Int val2 = overflow::saturatingMul(_coeffs[i], ub);
    total = overflow::saturatingAdd(total, std::max(val1, val2));
  }
  if (total <= _bound) {
    return;
  }

  std::vector<search::SearchVar> searchVars;
  searchVars.reserve(outputVarNodes().size());

  for (const auto& vNode : outputVarNodes()) {
    assert(mapping.solverId(vNode) != propagation::NULL_ID);
    searchVars.emplace_back(mapping.solverId(vNode), vNode->constDomain());
  }

  if (_isBinary) {
    if (outputVarNodes().front()->isIntVar()) {
      mapping.setNeighborhood(
          ptrConst(), std::make_shared<
                    search::neighborhoods::BinaryLinLeNeighborhood<false>>(
                    std::vector<Int>{_coeffs}, std::move(searchVars), _bound));
      return;
    }
    mapping.setNeighborhood(
        ptrConst(),
        std::make_shared<search::neighborhoods::BinaryLinLeNeighborhood<true>>(
            std::vector<Int>{_coeffs}, std::move(searchVars), _bound));
    return;
  }
  mapping.setNeighborhood(
      ptrConst(), std::make_shared<search::neighborhoods::IntLinLeNeighborhood>(
                std::vector<Int>{_coeffs}, std::move(searchVars), _bound));
}

std::string LinLeImplicitNode::dotLangIdentifier() const { return "lin_le"; }

}  // namespace atlantis::invariantgraph
