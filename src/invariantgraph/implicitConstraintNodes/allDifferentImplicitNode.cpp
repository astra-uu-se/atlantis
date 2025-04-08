#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"

#include <algorithm>
#include <limits>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/search/neighborhoods/allDifferentNonUniformNeighborhood.hpp"
#include "atlantis/search/neighborhoods/allDifferentUniformNeighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

AllDifferentImplicitNode::AllDifferentImplicitNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& inputVars)
    : ImplicitConstraintNode(graph, std::move(inputVars)) {}

void AllDifferentImplicitNode::init(InvariantNodeId id) {
  ImplicitConstraintNode::init(id);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

std::shared_ptr<search::neighborhoods::Neighborhood>
AllDifferentImplicitNode::createNeighborhood() {
  if (outputVarNodeIds().size() <= 1) {
    return nullptr;
  }
  bool hasSameDomain = true;
  assert(!outputVarNodeIds().empty());

  const auto domain = invariantGraphConst()
                          .varNodeConst(outputVarNodeIds().front())
                          .constDomain();

  for (size_t i = 1; i < outputVarNodeIds().size(); ++i) {
    if ((*invariantGraphConst()
              .varNodeConst(outputVarNodeIds().at(i))
              .constDomain()) != (*domain)) {
      hasSameDomain = false;
      break;
    }
  }

  std::vector<search::SearchVar> searchVars;
  // "malloc(): invalid size (unsorted)" exception: don't reserve
  searchVars.reserve(outputVarNodeIds().size());

  if (hasSameDomain) {
    for (const auto& nId : outputVarNodeIds()) {
      auto& varNode = invariantGraph().varNode(nId);
      assert(varNode.varId() != propagation::NULL_ID);
      searchVars.emplace_back(varNode.varId(), varNode.domain());
      varNode.setDomainType(DomainType::DOM_NONE);
    }
    return std::make_shared<
        search::neighborhoods::AllDifferentUniformNeighborhood>(
        std::move(searchVars));
  }
  Int domainLb = std::numeric_limits<Int>::max();
  Int domainUb = std::numeric_limits<Int>::min();
  for (const auto& nId : outputVarNodeIds()) {
    auto& varNode = invariantGraph().varNode(nId);
    searchVars.emplace_back(varNode.varId(), varNode.domain());
    domainLb = std::min<Int>(domainLb, varNode.constDomain()->lowerBound());
    domainUb = std::max<Int>(domainUb, varNode.constDomain()->upperBound());
  }
  return std::make_shared<
      search::neighborhoods::AllDifferentNonUniformNeighborhood>(
      std::move(std::move(searchVars)), domainLb, domainUb);
}

std::string AllDifferentImplicitNode::dotLangIdentifier() const {
  return "all_different";
}

}  // namespace atlantis::invariantgraph
